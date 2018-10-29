#include "wave_engine.h"

namespace WaveSimulation {

WaveEngine::WaveEngine() {
	ctDone = (bool *) calloc(sizeof(bool), MAX_NUMBER_OF_THREADS);
	coThreads = (std::thread *) calloc(sizeof(std::thread),
	MAX_NUMBER_OF_THREADS);
	ctStruct = (coThreadStruct *) calloc(sizeof(coThreadStruct),
	MAX_NUMBER_OF_THREADS);
	mEndMutex = (std::mutex *) calloc(sizeof(std::mutex),
	MAX_NUMBER_OF_THREADS);
	mEndCond = (std::condition_variable *) calloc(
			sizeof(std::condition_variable),
			MAX_NUMBER_OF_THREADS);

	osc_active = (bool *) calloc(sizeof(bool), MAX_NUMBER_OF_OSCILLATORS);
	osc_source = (OscillatorSource *) calloc(sizeof(OscillatorSource),
	MAX_NUMBER_OF_OSCILLATORS);
	osc_period = (double *) calloc(sizeof(double), MAX_NUMBER_OF_OSCILLATORS);
	for (unsigned int i = 0; i < MAX_NUMBER_OF_OSCILLATORS; i++)
		osc_period[i] = 30.0;
	osc_phase = (double *) calloc(sizeof(double), MAX_NUMBER_OF_OSCILLATORS);
	osc_amplitude = (double *) calloc(sizeof(double),
	MAX_NUMBER_OF_OSCILLATORS);
	for (unsigned int i = 0; i < MAX_NUMBER_OF_OSCILLATORS; i++)
		osc_amplitude[i] = 1.0;
	osc_move_period = (double *) calloc(sizeof(double),
	MAX_NUMBER_OF_OSCILLATORS);
	for (unsigned int i = 0; i < MAX_NUMBER_OF_OSCILLATORS; i++)
		osc_move_period[i] = 800.0;
	osc_locations = (unsigned int **) calloc(sizeof(unsigned int *),
	MAX_NUMBER_OF_OSCILLATORS);
	osc_locations_size = (unsigned int *) calloc(sizeof(unsigned int),
	MAX_NUMBER_OF_OSCILLATORS);
	osc_location1_p = (Point *) calloc(sizeof(Point),
	MAX_NUMBER_OF_OSCILLATORS);
	osc_location2_p = (Point *) calloc(sizeof(Point),
	MAX_NUMBER_OF_OSCILLATORS);

	setPool(size);
	setCoThreads(0);
	MainT = std::thread(MainThreadFunc, this);

}

WaveEngine::~WaveEngine() {
	disposing = true;
	work_now = false;

	mutex.lock();
	sendOrderToCT(Destroy);
	for (unsigned int i = 0; i < numOfThreads; i++)
		if (coThreads[i].joinable())
			coThreads[i].join();
	mutex.unlock();
	if (MainT.joinable())
		MainT.join();
	//pthread_mutex_destroy(&mutex);

	//for (int i = 0; i < MAX_NUMBER_OF_THREADS; i++) {
	//	pthread_mutex_destroy(&mEndMutex[i]);
	//	pthread_cond_destroy(&mEndCond[i]);
	//}
	//pthread_mutex_destroy(&mStartMutex);
	//pthread_cond_destroy(&mStartCond);

	//pthread_mutex_destroy(&mCout);

	free(mEndMutex);
	free(mEndCond);
#if defined(WAVE_ENGINE_SPRING_MODEL)
	free(pd);
	free(pd_previous);
	free(pdv);
#elif defined(WAVE_ENGINE_X_MODEL)
#endif
	free(pdl);
	free(pdm);
	free(pd_static);
	free(pd_location_info);
	free(bitmap_data);

	free(ctDone);
	free(coThreads);
	free(ctStruct);

	free(osc_active);
	free(osc_source);
	free(osc_period);
	free(osc_phase);
	free(osc_amplitude);
	free(osc_move_period);
	for (unsigned int i = 0; i < MAX_NUMBER_OF_OSCILLATORS; i++)
		free(osc_locations[i]);
	free(osc_locations);
	free(osc_locations_size);
	free(osc_location1_p);
	free(osc_location2_p);
}
/*
 double ts_to_ms(timespec ts) {
 return (ts.tv_sec) * (double) 1e3 + (ts.tv_nsec) / (double) 1e6;
 }

 struct timespec ms_to_ts(double ms) {
 struct timespec ts;
 ts.tv_sec = (unsigned long) (ms / 1e3);
 ts.tv_nsec = (unsigned long) (fmod(ms, 1000) * 1e6);
 return ts;
 }
 */
void WaveEngine::sendOrderToCT(coThreadMission order) {
	ctMission = order;
	for (unsigned int i = 0; i < numOfThreads; i++) {
		mEndMutex[i].lock();
		ctDone[i] = false;
		mEndMutex[i].unlock();
	}
	mStartMutex.lock();

	mStartCond.notify_all();
	mStartMutex.unlock();
}

void WaveEngine::waitForCT() {
	for (unsigned int i = 0; i < numOfThreads; i++) {
		{
			std::unique_lock<std::mutex> unique_lock(mEndMutex[i]);
			while (work_now && !ctDone[i]) {

				mEndCond[i].wait_for(unique_lock, std::chrono::seconds(3));

			}
		}
	}
}

void * WaveEngine::MainThreadFunc(void * data) {
	WaveEngine * waveEngine = (WaveEngine *) data;

	// It is challenging to be accurate with the limiter at high speed calculations. If we use thread.sleep() function,
	// number of iterations per second (IPS) will be quite lower than the one specified for the limiter at high speeds.
	// That is because, the sleep times are not perfect and the program doesn't always get enough CPU power.
	// So we take another approach. We calculate the number of the calculations that should have been done since the epoch
	// by checking the current time-stamp and decide if the main thread should yield or do another iteration. That way, the
	// calculation periods will not be always homogeneous and there will be bursting problem but at least the requested
	// number of calculations will be provided.

	//struct timespec time_current, time_log_previous, time_start;
	std::chrono::steady_clock::time_point time_previous, time_current,
			time_log_previous;
	time_previous = time_log_previous = time_current =
			std::chrono::steady_clock::now();
	time_previous = time_current - std::chrono::seconds(1); // Make sure that a new session starts first.
	double numOfCalcs = 0; // For statistics, how many calculations have been done so far?
	double numOfPaints = 0; // For statistics, how many paintings have been done so far?
	unsigned int calcNeeded = 0; // How many calculations should have been done since epoch?
	unsigned int paintNeeded = 0; // How many paintings should have been done since epoch?
	while (!waveEngine->disposing) {

		while (waveEngine->work_now) {

			// Reset the counters every second. If calcDone can't keep up with the calcNeeded for some reason (lag, etc.)
			// and the difference between them is too big, then the program will ignore the limiter for a long time,
			// trying to reach the calcNeeded (too long bursts). With this session thing, the program will ignore the limiter
			// at most for a second.
			time_current = std::chrono::steady_clock::now();
			if (std::chrono::duration_cast<std::chrono::duration<double>>(
					time_current - time_previous).count() > 1.0) {
				// Starting a new session.
				time_previous = time_current;
				calcNeeded = (unsigned int) (std::chrono::duration_cast<
						std::chrono::duration<double>>(
						time_current.time_since_epoch()).count()
						* waveEngine->IPS);
				paintNeeded = (unsigned int) (std::chrono::duration_cast<
						std::chrono::duration<double>>(
						time_current.time_since_epoch()).count()
						* waveEngine->FPS);
				waveEngine->calcDone = calcNeeded;
				waveEngine->paintDone = paintNeeded;
			}
			if (waveEngine->work_now && waveEngine->calculationEnabled) {
				time_current = std::chrono::steady_clock::now();
				if ((waveEngine->IPS == 0
						|| (calcNeeded =
								(unsigned int) (std::chrono::duration_cast<
										std::chrono::duration<double>>(
										time_current.time_since_epoch()).count()
										* waveEngine->IPS))
								> waveEngine->calcDone)) {
					waveEngine->mutex.lock();
					// Prepare temporary variables for multi-threaded operation.
					// With the help of these variables, we avoid multiple for-loops.
					memcpy(waveEngine->pd_previous, waveEngine->pd,
							sizeof(double_t) * waveEngine->sizesize);
					waveEngine->average_height = 0;
#if defined(WAVE_ENGINE_SPRING_MODEL)
					waveEngine->sendOrderToCT(CalculateForces);
#elif defined(WAVE_ENGINE_X_MODEL)
#endif
					waveEngine->waitForCT();
					waveEngine->average_height /= waveEngine->sizesized;

					numOfCalcs++;
					waveEngine->calcDone++;
					waveEngine->calcCounter++;
					waveEngine->mutex.unlock();

				}

			}

			if (waveEngine->work_now && waveEngine->renderEnabled) {
				time_current = std::chrono::steady_clock::now();
				if (waveEngine->FPS == 0
						|| (paintNeeded =
								(unsigned int) (std::chrono::duration_cast<
										std::chrono::duration<double>>(
										time_current.time_since_epoch()).count()
										* waveEngine->FPS))
								> waveEngine->paintDone) {
					waveEngine->mutex.lock();
					waveEngine->sendOrderToCT(CalculateColors);

					waveEngine->waitForCT();

					numOfPaints++;
					waveEngine->paintDone++;
					waveEngine->mutex.unlock();

					if (waveEngine->renderCallback != NULL)
						(*waveEngine->renderCallback)(waveEngine->bitmap_data,
								waveEngine->sizesize * 3,
								waveEngine->extra_data);

				}

			}

			if (waveEngine->work_now) {
				time_current = std::chrono::steady_clock::now();
				if (std::chrono::duration_cast<std::chrono::duration<double>>(
						time_current - time_log_previous).count()
						>= waveEngine->performanceLogInterval / 1000.0) {
					time_log_previous = std::chrono::steady_clock::now();
					double perf_interval =
							(double) waveEngine->performanceLogInterval;
					waveEngine->mCout.lock();
					std::cout << "Iterations & Paints per second:	"
							<< 1000.0 / (perf_interval / numOfCalcs) << "	"
							<< 1000.0 / (perf_interval / numOfPaints)
							<< std::endl;
					numOfCalcs = 0;
					numOfPaints = 0;
					waveEngine->mCout.unlock();

				}
			}
			if ((!waveEngine->powerSaveMode
					&& (waveEngine->FPS == 0 || waveEngine->IPS == 0))
					|| (waveEngine->calculationEnabled
							&& waveEngine->calcDone < calcNeeded)
					|| (waveEngine->renderEnabled
							&& waveEngine->paintDone < paintNeeded)) {
				// In a hurry
				std::this_thread::yield();
			} else {
				waveEngine->mutex.lock();
				waveEngine->sendOrderToCT(Pause);
				waveEngine->mutex.unlock();
				if (waveEngine->powerSaveMode)
					std::this_thread::sleep_for(
							std::chrono::milliseconds(waveEngine->TDelay));
				else
					std::this_thread::yield();
			}

		}

		std::this_thread::sleep_for(
				std::chrono::milliseconds(waveEngine->TDelay));
	}
	return 0;
}

bool WaveEngine::getOscillatorEnabled(unsigned int oscillatorId) {
	if (oscillatorId < MAX_NUMBER_OF_OSCILLATORS)
		return osc_active[oscillatorId];
	else
		return false;

}
void WaveEngine::setOscillatorEnabled(unsigned int oscillatorId, bool enabled) {
	if (oscillatorId < MAX_NUMBER_OF_OSCILLATORS) {
		mutex.lock();
		osc_active[oscillatorId] = enabled;
		mutex.unlock();
	}
}

OscillatorSource WaveEngine::getOscillatorSource(unsigned int oscillatorId) {
	if (oscillatorId < MAX_NUMBER_OF_OSCILLATORS)
		return osc_source[oscillatorId];
	else
		return PointSource;
}
void WaveEngine::setOscillatorSource(unsigned int oscillatorId,
		OscillatorSource oscillatorSource) {
	if (oscillatorId < MAX_NUMBER_OF_OSCILLATORS) {
		mutex.lock();
		osc_source[oscillatorId] = oscillatorSource;
		updateOscLocIndices(oscillatorId);
		mutex.unlock();
	}
}

double WaveEngine::getOscillatorPeriod(unsigned int oscillatorId) {
	if (oscillatorId < MAX_NUMBER_OF_OSCILLATORS)
		return osc_period[oscillatorId];
	else
		return -1;
}
void WaveEngine::setOscillatorPeriod(unsigned int oscillatorId, double period) {
	if (oscillatorId < MAX_NUMBER_OF_OSCILLATORS && period >= 1) {
		mutex.lock();
		osc_period[oscillatorId] = period;
		mutex.unlock();
	}
}

double WaveEngine::getOscillatorPhase(unsigned int oscillatorId) {
	if (oscillatorId < MAX_NUMBER_OF_OSCILLATORS)
		return osc_phase[oscillatorId];
	else
		return -1;
}
void WaveEngine::setOscillatorPhase(unsigned int oscillatorId, double phase) {
	if (oscillatorId < MAX_NUMBER_OF_OSCILLATORS) {
		mutex.lock();
		osc_phase[oscillatorId] = phase;
		mutex.unlock();
	}
}

double WaveEngine::getOscillatorAmplitude(unsigned int oscillatorId) {
	if (oscillatorId < MAX_NUMBER_OF_OSCILLATORS)
		return osc_amplitude[oscillatorId];
	else
		return -1;
}
void WaveEngine::setOscillatorAmplitude(unsigned int oscillatorId,
		double amplitude) {
	if (oscillatorId < MAX_NUMBER_OF_OSCILLATORS) {
		mutex.lock();
		osc_amplitude[oscillatorId] = amplitude;
		mutex.unlock();
	}
}

double WaveEngine::getOscillatorMovePeriod(unsigned int oscillatorId) {
	if (oscillatorId < MAX_NUMBER_OF_OSCILLATORS)
		return osc_move_period[oscillatorId];
	else
		return -1;
}
void WaveEngine::setOscillatorMovePeriod(unsigned int oscillatorId,
		double movePeriod) {
	if (oscillatorId < MAX_NUMBER_OF_OSCILLATORS && movePeriod >= 1) {
		mutex.lock();
		osc_move_period[oscillatorId] = movePeriod;
		mutex.unlock();
	}
}

Point WaveEngine::getOscillatorLocation(unsigned int oscillatorId,
		unsigned int locationId) {
	if (oscillatorId < MAX_NUMBER_OF_OSCILLATORS) {
		if (locationId == 0)
			return osc_location1_p[oscillatorId];
		else if (locationId == 1)
			return osc_location2_p[oscillatorId];
		else
			return Point(-1, -1);
	} else
		return Point(-1, -1);

}
void WaveEngine::setOscillatorLocation(unsigned int oscillatorId,
		unsigned int locationId, Point location) {
	if (oscillatorId < MAX_NUMBER_OF_OSCILLATORS && locationId <= 1) {
		mutex.lock();
		if (locationId == 0)
			osc_location1_p[oscillatorId] = location;
		else
			osc_location2_p[oscillatorId] = location;

		updateOscLocIndices(oscillatorId);
		mutex.unlock();
	}

}

Point WaveEngine::getOscillatorRealLocation(unsigned int oscillatorId) {
	if (oscillatorId < MAX_NUMBER_OF_OSCILLATORS) {
		double ratio_1;
		switch (osc_source[oscillatorId]) {
		case PointSource:
			return osc_location1_p[oscillatorId];
		case LineSource:
			return Point(
					(osc_location1_p[oscillatorId].x
							+ osc_location2_p[oscillatorId].x) / 2,
					(osc_location1_p[oscillatorId].y
							+ osc_location2_p[oscillatorId].y) / 2);
		case MovingPointSource:
			ratio_1 = fmod(calcCounter, osc_move_period[oscillatorId])
					/ osc_move_period[oscillatorId];
			return Point(
					(1.0 - ratio_1) * osc_location1_p[oscillatorId].x
							+ ratio_1 * osc_location2_p[oscillatorId].x,
					(1.0 - ratio_1) * osc_location1_p[oscillatorId].y
							+ ratio_1 * osc_location2_p[oscillatorId].y);
		default:
			return Point(-1, -1);
		}
	} else
		return Point(-1, -1);
}

double WaveEngine::getLossRatio() {
	return loss;
}

void WaveEngine::setLossRatio(double loss) {
	mutex.lock();
	this->loss = clamp(loss, 0.0, 1.0);
	setLossRatio();
	mutex.unlock();
}

double WaveEngine::getFramesPerSecond() {
	return FPS;
}

void WaveEngine::setFramesPerSecond(double framesPerSecond) {
	mutex.lock();
	this->FPS = clamp(framesPerSecond, 0, framesPerSecond);
	paintDone = (unsigned int) (std::chrono::duration_cast<
			std::chrono::duration<double>>(
			std::chrono::steady_clock::now().time_since_epoch()).count() * FPS);
	mutex.unlock();
}

double WaveEngine::getIterationsPerSecond() {
	return IPS;
}

void WaveEngine::setIterationsPerSecond(double iterationsPerSecond) {
	mutex.lock();
	this->IPS = clamp(iterationsPerSecond, 0, iterationsPerSecond);
	calcDone = (unsigned int) (std::chrono::duration_cast<
			std::chrono::duration<double>>(
			std::chrono::steady_clock::now().time_since_epoch()).count() * IPS);
	mutex.unlock();
}

unsigned int WaveEngine::getNumberOfThreads() {
	return numOfThreads;
}

void WaveEngine::setNumberOfThreads(unsigned int numberOfThreads) {
	mutex.lock();
	unsigned int oldNumOfThreads = numOfThreads;
	this->numOfThreads = clamp(numberOfThreads, 1, MAX_NUMBER_OF_THREADS);
	setCoThreads(oldNumOfThreads);
	mutex.unlock();
}

unsigned int WaveEngine::getThreadDelay() {
	return TDelay;
}

void WaveEngine::setThreadDelay(unsigned int threadDelay) {
	mutex.lock();
	this->TDelay = clamp(threadDelay, 0, 1000);
	mutex.unlock();
}

bool WaveEngine::getRenderEnabled() {
	return renderEnabled;
}

void WaveEngine::setRenderEnabled(bool renderEnabled) {
	mutex.lock();
	this->renderEnabled = renderEnabled;
	mutex.unlock();
}

bool WaveEngine::getCalculationEnabled() {
	return calculationEnabled;
}

void WaveEngine::setCalculationEnabled(bool calculationEnabled) {
	mutex.lock();
	this->calculationEnabled = calculationEnabled;
	mutex.unlock();
}

bool WaveEngine::getLogPerformance() {
	return logPerformance;
}

void WaveEngine::setLogPerformance(bool logPerformance) {
	mutex.lock();
	this->logPerformance = logPerformance;
	mutex.unlock();
}

bool WaveEngine::getPowerSaveMode() {
	return powerSaveMode;
}

void WaveEngine::setPowerSaveMode(bool powerSaveMode) {
	mutex.lock();
	this->powerSaveMode = powerSaveMode;
	mutex.unlock();
}

unsigned int WaveEngine::getPerformanceLogInterval() {
	return performanceLogInterval;
}

void WaveEngine::setPerformanceLogInterval(
		unsigned int performanceLogInterval) {
	mutex.lock();
	this->performanceLogInterval = clamp(performanceLogInterval, 0,
			performanceLogInterval);
	mutex.unlock();
}

double WaveEngine::getMassMapRangeHigh() {
	return massMapRangeHigh;
}

void WaveEngine::setMassMapRangeHigh(double massMapRangeHigh) {
	mutex.lock();
	this->massMapRangeHigh = clamp(massMapRangeHigh, 0, massMapRangeHigh);
	mutex.unlock();
}

double WaveEngine::getMassMapRangeLow() {
	return massMapRangeLow;
}

void WaveEngine::setMassMapRangeLow(double massMapRangeLow) {
	mutex.lock();
	this->massMapRangeLow = clamp(massMapRangeLow, 0, massMapRangeLow);
	mutex.unlock();
}

bool WaveEngine::getShowMassMap() {
	return massMap;
}

void WaveEngine::setShowMassMap(bool showMassMap) {
	mutex.lock();
	this->massMap = showMassMap;
	mutex.unlock();
}

unsigned int WaveEngine::getSize() {
	return size;
}

void WaveEngine::setSize(unsigned int size) {
	mutex.lock();
	unsigned int oldsize = this->size;
	this->size = clamp(size, 1, size);
	sizesize = size * size;
	sizesized = sizesize;
	setPool(oldsize);
	setCoThreads(numOfThreads);
	mutex.unlock();
}

double WaveEngine::getAbsorberLossRatio() {
	return max_loss;
}

void WaveEngine::setAbsorberLossRatio(double absorberLoss) {
	mutex.lock();
	this->max_loss = clamp(absorberLoss, 0, 1.0);
	setLossRatio();
	mutex.unlock();
}

unsigned int WaveEngine::getAbsorberThickness() {
	return absorb_offset;
}

void WaveEngine::setAbsorberThickness(unsigned int absorberThickness) {
	mutex.lock();
	this->absorb_offset = clamp(absorberThickness, 0, size / 2);
	setLossRatio();
	mutex.unlock();
}

bool WaveEngine::getShiftParticlesEnabled() {
	return shifting;
}

void WaveEngine::setShiftParticlesEnabled(bool shiftParticlesEnabled) {
	mutex.lock();
	this->shifting = shiftParticlesEnabled;
	mutex.unlock();
}

bool WaveEngine::getAbsorberEnabled() {
	return absorberEnabled;
}

void WaveEngine::setAbsorberEnabled(bool absorberEnabled) {
	mutex.lock();
	this->absorberEnabled = absorberEnabled;
	setLossRatio();
	mutex.unlock();
}

WaveEngine::RenderCallback WaveEngine::getRenderCallback() {
	return renderCallback;
}

void WaveEngine::setRenderCallback(RenderCallback renderCallback,
		void * extra_data) {
	mutex.lock();
	this->renderCallback = renderCallback;
	this->extra_data = extra_data;
	mutex.unlock();
}

void * WaveEngine::getExtraData() {
	return extra_data;
}

bool WaveEngine::getExtremeContrastEnabled() {
	return extremeContrastEnabled;
}

void WaveEngine::setExtremeContrastEnabled(bool extremeContrastEnabled) {
	mutex.lock();
	this->extremeContrastEnabled = extremeContrastEnabled;
	mutex.unlock();
}

unsigned int WaveEngine::getAmplitudeMultiplier() {
	return amplitudeMultiplier;
}

void WaveEngine::setAmplitudeMultiplier(unsigned int amplitudeMultiplier) {
	mutex.lock();
	this->amplitudeMultiplier = amplitudeMultiplier;
	mutex.unlock();
}

Color WaveEngine::getCrestColor() {
	return crestColor;
}

void WaveEngine::setCrestColor(Color crestColor) {
	mutex.lock();
	this->crestColor = crestColor;
	mutex.unlock();
}

Color WaveEngine::getTroughColor() {
	return troughColor;
}

void WaveEngine::setTroughColor(Color troughColor) {
	mutex.lock();
	this->troughColor = troughColor;
	mutex.unlock();
}

Color WaveEngine::getStaticColor() {
	return staticColor;
}

void WaveEngine::setStaticColor(Color staticColor) {
	mutex.lock();
	this->staticColor = staticColor;
	mutex.unlock();
}

bool WaveEngine::lock() {
	if (locked)
		return false;
	locked = true;
	mutex.lock();
	return true;
}

bool WaveEngine::unlock() {
	mutex.unlock();
	locked = false;
	return true;
}

void * WaveEngine::getData(ParticleAttribute particleAttribute) {
	if (!locked)
		return nullptr;
	switch (particleAttribute) {
	case Fixity:
		return pd_static;
	case Loss:
		return pdl;
#if defined(WAVE_ENGINE_SPRING_MODEL)
	case Height:
		return pd;
	case Velocity:
		return pdv;
#elif defined(WAVE_ENGINE_X_MODEL)
#endif
	case Mass:
		return pdm;
	default:
		return nullptr;
	}
}

void WaveEngine::start() {
	mutex.lock();
	work_now = true;
	mutex.unlock();
}

void WaveEngine::stop() {
	mutex.lock();
	work_now = false;
	mutex.unlock();
}

bool WaveEngine::isWorking() {
	return work_now;
}
#if defined(WAVE_ENGINE_SPRING_MODEL)
bool WaveEngine::calculateForces(const unsigned int firstIndex,
		const unsigned int count) {
#elif defined(WAVE_ENGINE_X_MODEL)
#endif
	// Check if the parameters are valid.
	if (firstIndex + count > sizesize || count < 1)
		return false;

	const unsigned int fplusc = firstIndex + count;
	double local_average_height = 0;
	for (unsigned int index = firstIndex; index < fplusc; index++) {
		// If this is a static particle, it will not move at all. Continue with the next particle.
		if (pd_static[index]) {
#if defined(WAVE_ENGINE_SPRING_MODEL)
			pd[index] = 0;
#elif defined(WAVE_ENGINE_X_MODEL)
#endif
			continue;
		}

#if defined(WAVE_ENGINE_SPRING_MODEL)

		// Think that each particle is connected to its 4 neighbor particles (north,south,east,west).
		// The connection is provided by a spring whose constant is k = 1
		// The particles can only move up and down meaning that only their altitude/height can change.
		// Find forces exerted by springs. F = -kx in this case. x is the distance to the rest point of that spring.
		// Springs are connected in parallel. So forces are simply added.
		// After finding the total force, from F = ma -> F/m = a, find the acceleration, and add it to the velocity.

		double heights = 0;
		double num_of_parts = 0;

		if (pd_location_info[index].top && !pd_static[index - size]) {
			heights += pd_previous[index - size];
			num_of_parts++;
		}

		if (pd_location_info[index].right && !pd_static[index + 1]) {
			heights += pd_previous[index + 1];
			num_of_parts++;
		}

		if (pd_location_info[index].left && !pd_static[index - 1]) {
			heights += pd_previous[index - 1];
			num_of_parts++;
		}

		if (pd_location_info[index].bottom && !pd_static[index + size]) {
			heights += pd_previous[index + size];
			num_of_parts++;
		}

		if (num_of_parts != 0) {
			double average_height = heights / num_of_parts;
			double velocity_gain = (heights - pd_previous[index] * num_of_parts)
					/ pdm[index];

			// Limit velocity to prevent chaos. When the simulation resolution is low enough,
			// a particle can gain a velocity which would carry it to such a point in single iteration
			// that it would be impossible if the simulation resolution would be higher. If not prevented,
			// the particle will vastly increase its distance from the equilibrium point.
			// This is a positive feedback case resulting in violation of conservation of energy and, ultimately, the chaos.
			if (velocity_gain > 0)
				pd[index] += pdv[index] += clamp(velocity_gain, velocity_gain,
						(average_height - pd_previous[index]) );
			else
				pd[index] += pdv[index] += clamp(velocity_gain,
						(average_height - pd_previous[index]) ,
						velocity_gain);
		}

		// Loss takes place.

		if (pdl[index] > 0.0) {
			// Reduce the kinetic energy. The kinetic energy is 0.5 times mass times velocity squared.
			// With this equation, we can derive the velocity for the reduced kinetic energy.
			double kinetic_energy = (0.5 * pdm[index] * pow(pdv[index], 2));
			// Multiply the energy with one minus loss ratio and find the velocity for that kinetic energy.
			// Leaving the velocity on the left side results in the following.
			pdv[index] = sqrt(
					2 * kinetic_energy * (1.0 - pdl[index]) / pdm[index])
					* sgn(pdv[index]);

			// Reducing potential energy triggers chaos?
			/*double height_difference = pd_previous[index]
			 - heights / num_of_parts; // Current height minus equilibrium height.
			 // Reduce the potential energy. The potential energy for this model is 0.5 times height difference squared.
			 // With this equation, we can derive the height for the reduced potential energy.
			 double potential_energy = (0.5 * pow(height_difference, 2.0));
			 // Multiply the energy with one minus loss ratio and find the height for that potential energy.
			 // Leaving the height difference on the left side results in the following.
			 pd[index] += sqrt(2 * potential_energy * (1.0 - pdl[index]))
			 * sgn(height_difference) - height_difference;*/
		}

		local_average_height += pd[index];
#elif defined(WAVE_ENGINE_X_MODEL)
#endif
	}
	average_height += local_average_height;
	// Process oscillators
	for (unsigned int i = 0; i < MAX_NUMBER_OF_OSCILLATORS; i++) {
		if (osc_active[i]) {
			double osc_height = osc_amplitude[i]
					* sin(
							osc_phase[i] * M_PI / 180.0
									+ 2.0 * M_PI
											* fmod(calcCounter, osc_period[i])
											/ osc_period[i]);

			switch (osc_source[i]) {
			case PointSource:
#if defined(WAVE_ENGINE_SPRING_MODEL)
				average_height += pd[osc_locations[i][0]] = osc_height;
				pdv[osc_locations[i][0]] = 0;
#elif defined(WAVE_ENGINE_X_MODEL)
#endif
				break;
			case LineSource:
				for (unsigned int j = 0; j < osc_locations_size[i]; j++) {
#if defined(WAVE_ENGINE_SPRING_MODEL)
					average_height += pd[osc_locations[i][j]] = osc_height;
					pdv[osc_locations[i][j]] = 0;
#elif defined(WAVE_ENGINE_X_MODEL)
#endif
				}
				break;
			case MovingPointSource:
				double ratio_1 = fmod(calcCounter, osc_move_period[i])
						/ osc_move_period[i];
				Point cur_point = Point(
						(1.0 - ratio_1) * osc_location1_p[i].x
								+ ratio_1 * osc_location2_p[i].x,
						(1.0 - ratio_1) * osc_location1_p[i].y
								+ ratio_1 * osc_location2_p[i].y);
				unsigned int cur_index = (unsigned int) cur_point.x
						+ size * (unsigned int) cur_point.y;
#if defined(WAVE_ENGINE_SPRING_MODEL)
				average_height += pd[cur_index] = osc_height;
				pdv[cur_index] = 0;
#elif defined(WAVE_ENGINE_X_MODEL)
#endif
				break;
			}

		}
	}
	return true;
}

void WaveEngine::setLossRatio() {
	if (absorberEnabled) {
		// We will fill "pdl" array with "loss" then we will deal with elements near to window boundaries.

		// Since we want the loss to increase towards the edges, "max_loss" can't be smaller than "loss".
		if (max_loss < loss) {
			// The only thing to do is to fill "pdf" array with "loss" in this case.
			for (unsigned int i = 0; i < sizesize; i++)
				pdl[i] = loss;
			return;
		}

		// loss gain fields should not mix with each other. So the maximum offset is the middle-screen.
		if (absorb_offset >= size / 2) {
			absorb_offset = size / 2 - 1;
		}

		// This value is loss increasion rate per row/column. The increasion is linear.
		double dec = (max_loss - loss) / (double) absorb_offset;
		// This one stores the current loss.
		double cur = max_loss;

		// First, we fill "pdl" array with "loss".
		for (unsigned int i = 0; i < sizesize - 1; i++)
			pdl[i] = loss;

		// This loop sets up the loss values for the top.
		for (unsigned int off = 0; off <= absorb_offset; off++) {
			// Process each row/column from the edge to the offset.
			for (unsigned int x = off; x < size - off; x++) {
				// Process each loss element in the current row/column
				pdl[x + off * size] = cur;
			}
			cur -= dec;
		}

		cur = loss; // Reset the current loss.

		// This loop sets up the loss values for the bottom.
		for (unsigned int off = 0; off <= absorb_offset; off++) {
			for (unsigned int x = absorb_offset - off;
					x < size - (absorb_offset - off); x++) {
				pdl[x + off * size + size * (size - absorb_offset - 1)] = cur;
			}
			cur += dec;
		}

		cur = loss;

		// This loop sets up the loss values for the left.
		for (unsigned int off = 0; off <= absorb_offset; off++) {
			for (unsigned int x = absorb_offset - off;
					x < size - (absorb_offset - off); x++) {
				pdl[x * size + (absorb_offset - off)] = cur;
			}
			cur += dec;
		}

		cur = loss;

		// This loop sets up the loss values for the right.
		for (unsigned int off = 0; off <= absorb_offset; off++) {
			for (unsigned int x = absorb_offset - off;
					x < size - (absorb_offset - off); x++) {
				pdl[x * size + off + size - absorb_offset - 1] = cur;
			}
			cur += dec;
		}
	} else {
		// The only thing to do is to fill "pdl" array with "loss" in this case.
		for (unsigned int i = 0; i < sizesize; i++)
			pdl[i] = loss;
	}
}

bool WaveEngine::paintBitmap(const unsigned int firstIndex,
		const unsigned int count, uint8_t * rgbdata) {
	// Check if the parameters are valid
	if (count < 1 || firstIndex + count > sizesize || firstIndex > sizesize)
		return false;

	// Render the region that is associated with this thread.
	for (unsigned int index = firstIndex; index < firstIndex + count; index++) {
		if (!massMap) {

			if (pd_static[index]) {

				rgbdata[index * 3] = staticColor.r;
				rgbdata[index * 3 + 1] = staticColor.g;
				rgbdata[index * 3 + 2] = staticColor.b;

			} else {
				// This value is the 'brightness' of the height.
#if defined(WAVE_ENGINE_SPRING_MODEL)
				double pdx = pd[index] - (shifting ? average_height : 0);
#elif defined(WAVE_ENGINE_X_MODEL)
#endif
				if (extremeContrastEnabled) {
					if (pdx > 0) {

						rgbdata[index * 3] = crestColor.r;
						rgbdata[index * 3 + 1] = crestColor.g;
						rgbdata[index * 3 + 2] = crestColor.b;

					}

					else if (pdx < 0) {

						rgbdata[index * 3] = troughColor.r;
						rgbdata[index * 3 + 1] = troughColor.g;
						rgbdata[index * 3 + 2] = troughColor.b;

					} else if (pdx == 0) {

						rgbdata[index * 3] = (uint8_t) ((crestColor.r
								+ troughColor.r) / 2.0);
						rgbdata[index * 3 + 1] = (uint8_t) ((crestColor.g
								+ troughColor.g) / 2.0);
						rgbdata[index * 3 + 2] = (uint8_t) ((crestColor.b
								+ troughColor.b) / 2.0);

					}

				} else {
					double bright = (clamp(pdx * amplitudeMultiplier, -1.0, 1.0)
							+ 1.0) * 127.0;

					double brightr1 = bright / 255.0;
					double brightr2 = 1.0 - brightr1;

					rgbdata[index * 3] = crestColor.r * brightr1
							+ troughColor.r * brightr2;
					rgbdata[index * 3 + 1] = crestColor.g * brightr1
							+ troughColor.g * brightr2;
					rgbdata[index * 3 + 2] = crestColor.b * brightr1
							+ troughColor.b * brightr2;
				}
			}

		} else {
			/* RGB route (linear transition)

			 R		 G		 B
			 --------------------
			 0.0		0.0		0.0
			 0.0		0.0		0.5
			 0.5		0.0		0.0
			 1.0		0.5		0.0
			 1.0		1.0		0.5
			 1.0		1.0		1.0

			 This is similar to thermal image color scale.
			 Yields 128 * 5 - 4 = 636 colors.
			 */
			double massrange = massMapRangeHigh - massMapRangeLow;
			double colors = 128.0 * 5.0 - 4.0; // numFrames * numTrans - (numTrans - 1)
			if (massrange <= 0) {
				rgbdata[index * 3] = 0;
				rgbdata[index * 3 + 1] = 0;
				rgbdata[index * 3 + 2] = 0;
				return false;
			}
			uint16_t color = round(
					(clamp(pdm[index], massMapRangeLow, massMapRangeHigh)
							- massMapRangeLow) * colors / massrange);
			if (color < 128) {
				rgbdata[index * 3] = 0;
				rgbdata[index * 3 + 1] = 0;
				rgbdata[index * 3 + 2] = color;
			} else if (color < 128 * 2) {
				rgbdata[index * 3] = color & 0x7F;
				rgbdata[index * 3 + 1] = 0;
				rgbdata[index * 3 + 2] = 127;
			} else if (color < 128 * 3) {
				rgbdata[index * 3] = 128 + (color & 0x7F);
				rgbdata[index * 3 + 1] = color & 0x7F;
				rgbdata[index * 3 + 2] = 127 - (color & 0x7F);
			} else if (color < 128 * 4) {
				rgbdata[index * 3] = 255;
				rgbdata[index * 3 + 1] = 128 + (color & 0x7F);
				rgbdata[index * 3 + 2] = color & 0x7F;
			} else if (color < 128 * 5) {
				rgbdata[index * 3] = 255;
				rgbdata[index * 3 + 1] = 255;
				rgbdata[index * 3 + 2] = 128 + (color & 0x7F);
			}
		}
	}
	return true;
}

void WaveEngine::setPool(unsigned int oldsize) {

	unsigned int oldsizesize = oldsize * oldsize;

#if defined(WAVE_ENGINE_SPRING_MODEL)
	free(pd);
	pd = (double_t *) calloc(sizeof(double_t), sizesize);

	free(pd_previous);
	pd_previous = (double_t *) calloc(sizeof(double_t), sizesize);

	free(pdv);
	pdv = (double_t *) calloc(sizeof(double_t), sizesize);
#elif defined(WAVE_ENGINE_X_MODEL)
#endif

	uint8_t * pd_static_old = 0;
	bool pd_static_null = !pd_static;
	if (!pd_static_null) {
		pd_static_old = (uint8_t *) calloc(sizeof(uint8_t), oldsizesize);
		memcpy(pd_static_old, pd_static,
				sizeof(pd_static_old[0]) * oldsizesize);
	}

	free(pd_static);
	pd_static = (uint8_t *) calloc(sizeof(uint8_t), sizesize);

	free(pdl);
	pdl = (double_t *) calloc(sizeof(double_t), sizesize);

	double * pdm_old = 0;
	bool pdm_null = !pdm;
	if (!pdm_null) {
		pdm_old = (double *) calloc(sizeof(double), oldsizesize);
		memcpy(pdm_old, pdm, sizeof(pdm_old[0]) * oldsizesize);
	}

	free(pdm);
	pdm = (double_t *) calloc(sizeof(double_t), sizesize);

	if (pdm_null)
		for (unsigned int i = 0; i < sizesize; i++)
			pdm[i] = 5.0;

	free(bitmap_data);
	bitmap_data = (uint8_t *) calloc(sizeof(uint8_t), sizesize * 3);

	free(pd_location_info);
	pd_location_info = (ParticleLocationInfo *) calloc(
			sizeof(ParticleLocationInfo), sizesize);

	for (unsigned int i = 0; i < sizesize; i++) {
		const double iplus1modsz = (i + 1) % size;
		const double imodsz = i % size;

		if (i >= size)
			pd_location_info[i].top = true;
		if (iplus1modsz != 0)
			pd_location_info[i].right = true;
		if (imodsz != 0)
			pd_location_info[i].left = true;
		if (i < sizesize - size)
			pd_location_info[i].bottom = true;

		/*if (pd_location_info[i].top && pd_location_info[i].left
		 && pd_location_info[i].right && pd_location_info[i].bottom)
		 pd_location_info[i].num_of_neighbors = 8;
		 else if (((pd_location_info[i].left || pd_location_info[i].right)
		 && pd_location_info[i].top && pd_location_info[i].bottom)
		 || ((pd_location_info[i].bottom || pd_location_info[i].top)
		 && pd_location_info[i].right && pd_location_info[i].left))
		 pd_location_info[i].num_of_neighbors = 5;
		 else
		 pd_location_info[i].num_of_neighbors = 3;*/
	}

	// Resize static & mass map
	if (!pd_static_null || !pdm_null) {
		double stepsize = (double) oldsize / size;
		double stepsize_db2 = stepsize / 2;
		for (unsigned int y = 0; y < size; y++) {
			for (unsigned int x = 0; x < size; x++) {
				if (!pd_static_null)
					pd_static[x + size * y] =
							pd_static_old[(unsigned int) floor(
									(double) x * stepsize + stepsize_db2)
									+ oldsize
											* (unsigned int) floor(
													(double) y * stepsize
															+ stepsize_db2)];
				if (!pdm_null)
					pdm[x + size * y] = pdm_old[(unsigned int) floor(
							(double) x * stepsize + stepsize_db2)
							+ oldsize
									* (unsigned int) floor(
											(double) y * stepsize
													+ stepsize_db2)];
			}
		}
	}
	free(pd_static_old);
	free(pdm_old);

	// Re-locate oscillators
	for (unsigned int i = 0; i < MAX_NUMBER_OF_OSCILLATORS; i++) {
		osc_location1_p[i] = Point(
				osc_location1_p[i].x * (double) size / oldsize,
				osc_location1_p[i].y * (double) size / oldsize);

		osc_location2_p[i] = Point(
				osc_location2_p[i].x * (double) size / oldsize,
				osc_location2_p[i].y * (double) size / oldsize);

		updateOscLocIndices(i);
	}

	setLossRatio();

}

void WaveEngine::updateOscLocIndices(unsigned int oscillatorId) {

	Point p1 = osc_location1_p[oscillatorId];
	Point p2 = osc_location2_p[oscillatorId];
	double length, xoverl, yoverl;
	Point currentPoint;
	std::vector<unsigned int> indices;
	switch (osc_source[oscillatorId]) {
	case PointSource:
		if (p1.x >= 0 && p1.x < size && p1.y >= 0 && p1.y < size) {
			free(osc_locations[oscillatorId]);
			osc_locations_size[oscillatorId] = 1;
			osc_locations[oscillatorId] = (unsigned int *) calloc(
					sizeof(unsigned int), osc_locations_size[oscillatorId]);
			osc_locations[oscillatorId][0] = (unsigned int) p1.x
					+ size * (unsigned int) p1.y;
		}
		break;
	case LineSource:

		length = Point::dist(p1, p2);
		if (length == 0)
			return;
		xoverl = (p2.x - p1.x) / length;
		yoverl = (p2.y - p1.y) / length;

		for (double i = 0; i < length; i += 0.5) {
			currentPoint = Point(p1.x + (xoverl * i), p1.y + (yoverl * i));
			if (currentPoint.x < size && currentPoint.x >= 0
					&& currentPoint.y < size && currentPoint.y >= 0)
				indices.push_back(
						(unsigned int) floor(currentPoint.x)
								+ size * (unsigned int) floor(currentPoint.y));
		}
		free(osc_locations[oscillatorId]);
		osc_locations[oscillatorId] = nullptr;
		if ((osc_locations_size[oscillatorId] = indices.size()) > 0) {
			osc_locations[oscillatorId] = (unsigned int *) calloc(
					sizeof(unsigned int), osc_locations_size[oscillatorId]);
			std::copy(indices.begin(), indices.end(),
					osc_locations[oscillatorId]);
		}
		break;
	case MovingPointSource:
		free(osc_locations[oscillatorId]);
		osc_locations_size[oscillatorId] = 0;
		osc_locations[oscillatorId] = nullptr;
		break;
	}

}

void * WaveEngine::CoThreadFunc(void * data) {
	coThreadFuncArg *arg = (coThreadFuncArg*) data;
	WaveEngine * waveEngine = arg->waveEngine;
	int i = arg->index;
	delete arg;
	waveEngine->mCout.lock();
	std::cout << "co-thread[" << i << "] is going into loop" << std::endl;
	waveEngine->mCout.unlock();
	bool signal_main = false;

	while (waveEngine->ctMission != Destroy && !waveEngine->disposing) {

		signal_main = false;
		waveEngine->mEndMutex[i].lock();
		if (!waveEngine->ctDone[i]) {
#if defined(WAVE_ENGINE_SPRING_MODEL)
			if (waveEngine->ctMission == CalculateForces) {
				waveEngine->calculateForces(waveEngine->ctStruct[i].firstIndex,
						waveEngine->ctStruct[i].count);
				signal_main = true;
			}
#elif defined(WAVE_ENGINE_X_MODEL)
#endif
			else if (waveEngine->ctMission == CalculateColors) {
				waveEngine->paintBitmap(waveEngine->ctStruct[i].firstIndex,
						waveEngine->ctStruct[i].count, waveEngine->bitmap_data);
				signal_main = true;
			}

			if (signal_main) {
				//std::cout << "signaling main (" << i << ")" << std::endl;

				waveEngine->ctDone[i] = true;
				waveEngine->mEndCond[i].notify_one();

			}
		}
		waveEngine->mEndMutex[i].unlock();

		{
			std::unique_lock<std::mutex> unique_lock(waveEngine->mStartMutex);
			while (waveEngine->ctMission == Pause && !waveEngine->disposing
					&& waveEngine->work_now) {

				waveEngine->mStartCond.wait_for(unique_lock,
						std::chrono::seconds(1));

			}
		}

		if (!waveEngine->work_now)
			std::this_thread::sleep_for(
					std::chrono::milliseconds(waveEngine->TDelay));
	}

	waveEngine->mCout.lock();
	std::cout << "co-thread[" << i << "] is returning" << std::endl;
	waveEngine->mCout.unlock();
	return 0;
}

void WaveEngine::setCoThreads(unsigned int oldNumOfThreads) {

	sendOrderToCT(Destroy);

	for (unsigned int i = 0; i < oldNumOfThreads; i++)
		if (coThreads[i].joinable())
			coThreads[i].join();

	ctMission = Pause;

	bool size_plus_one = false;
	int partial_size = sizesize / numOfThreads;
	if (sizesize % numOfThreads > 0)
		size_plus_one = true;
	int curPart = 0;
	for (unsigned int i = 0; i < numOfThreads; i++) {
		if (i == numOfThreads - 1 && size_plus_one)
			partial_size++;
		ctStruct[i] = coThreadStruct(i, curPart, partial_size);
		coThreadFuncArg * funcarg = new coThreadFuncArg(this, i);

		coThreads[i] = std::thread(CoThreadFunc, funcarg);

		//if (pthread_create(&coThreads[i], NULL, CoThreadFunc, funcarg) == 0)
		curPart += partial_size;
	}
}
}

