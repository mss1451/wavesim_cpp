#include "project_handler.h"

namespace WaveSimulation {

ProjectHandler::ProjectHandler() {
}

ProjectHandler::ProjectHandler(string project_dir, bool create_dir) {
	this->project_dir = project_dir;
	setProjectsPath(project_dir, create_dir);
}

ProjectHandler::~ProjectHandler() {

}

string ProjectHandler::getProjectsPath() {
	return project_dir;
}

/* https://stackoverflow.com/a/18101042
 * Check whether a directory exists in a portable way (optionally create)*/
ProjectHandlerResult ProjectHandler::setProjectsPath(string path, bool create) {
	struct stat info;

	if (stat(path.c_str(), &info) != 0) {
		// Not found anything or can't access.
		// Try to create the directory if specified.
		if (create) {
			// 0777 is interpreted as octal, 777 as decimal. Don't use 777!
			// mkdir returns zero on success.
			if (!mkdir(path.c_str(), 0777)) {
				project_dir = path;
				return OK;
			}
			return ERROR;
		}
		return NOT_FOUND;
	} else if (info.st_mode & S_IFDIR) {
		// Found the directory.
		project_dir = path;
		return OK;
	} else {
		// Found something but it is not a directory.
		return NOT_FOUND;
	}

}

ProjectHandlerResult ProjectHandler::deleteProject(string projectName) {
	int result = unlink((project_dir + "/" + projectName).c_str());
	if (!result)
		return OK;
	else if (result == ENOENT)
		return NOT_FOUND;
	return ERROR;
}

ProjectHandlerResult ProjectHandler::saveProject(string projectName,
		string description, WaveEngine * waveEngine) {
	FILE * pFile;
	string path = project_dir + "/" + projectName;
	string path_uncompressed = path + ".uncompressed";

	if ((pFile = fopen(path_uncompressed.c_str(), "wb")) != nullptr) {
		bool wasWorking = waveEngine->isWorking();
		waveEngine->stop();
		uint64_t size = waveEngine->getSize();
		double_t loss = waveEngine->getLossRatio();
		double_t peak_loss = waveEngine->getAbsorberLossRatio();
		uint64_t absorber_thickness = waveEngine->getAbsorberThickness();
		uint8_t shift_to_origin = waveEngine->getShiftParticlesEnabled();
		uint8_t enable_absorber = waveEngine->getAbsorberEnabled();

		uint64_t osc_count = MAX_NUMBER_OF_OSCILLATORS;

		uint8_t * osc_active = (uint8_t *) calloc(osc_count, sizeof(uint8_t));
		uint8_t * osc_source = (uint8_t *) calloc(osc_count, sizeof(uint8_t));
		double_t * osc_period = (double_t *) calloc(osc_count,
				sizeof(double_t));
		double_t * osc_phase = (double_t *) calloc(osc_count, sizeof(double_t));
		double_t * osc_amplitude = (double_t *) calloc(osc_count,
				sizeof(double_t));
		double_t * osc_move_period = (double_t *) calloc(osc_count,
				sizeof(double_t));
		Point * osc_location1_p = (Point *) calloc(osc_count, sizeof(Point));
		Point * osc_location2_p = (Point *) calloc(osc_count, sizeof(Point));

		// Checkpoints are placed to check for write error.
		int result = OK;
		result |= smart_fwrite(&size, sizeof(size), 1, pFile);
		result |= smart_fwrite(&loss, sizeof(loss), 1, pFile);
		result |= smart_fwrite(&peak_loss, sizeof(peak_loss), 1, pFile);
		result |= smart_fwrite(&absorber_thickness, sizeof(absorber_thickness),
				1, pFile);
		result |= smart_fwrite(&shift_to_origin, sizeof(shift_to_origin), 1,
				pFile);
		result |= smart_fwrite(&enable_absorber, sizeof(enable_absorber), 1,
				pFile);

		result |= smart_fwrite(&osc_count, sizeof(osc_count), 1, pFile);

		if (result != OK) {
			cleanOnError(wasWorking, pFile, waveEngine, path_uncompressed);
			return ERROR;
		}

		for (uint64_t i = 0; i < osc_count; i++) {
			osc_active[i] = waveEngine->getOscillatorEnabled(i);
			switch (waveEngine->getOscillatorSource(i)) {
			case PointSource:
				osc_source[i] = 0;
				break;
			case LineSource:
				osc_source[i] = 1;
				break;
			case MovingPointSource:
				osc_source[i] = 2;
				break;
			}

			osc_period[i] = waveEngine->getOscillatorPeriod(i);
			osc_phase[i] = waveEngine->getOscillatorPhase(i);
			osc_amplitude[i] = waveEngine->getOscillatorAmplitude(i);
			osc_move_period[i] = waveEngine->getOscillatorMovePeriod(i);
			osc_location1_p[i] = waveEngine->getOscillatorLocation(i, 0);
			osc_location2_p[i] = waveEngine->getOscillatorLocation(i, 1);

			result |= smart_fwrite(&osc_active[i], sizeof(osc_active[0]), 1,
					pFile);
			result |= smart_fwrite(&osc_source[i], sizeof(osc_source[0]), 1,
					pFile);
			result |= smart_fwrite(&osc_period[i], sizeof(osc_period[0]), 1,
					pFile);
			result |= smart_fwrite(&osc_phase[i], sizeof(osc_phase[0]), 1,
					pFile);
			result |= smart_fwrite(&osc_amplitude[i], sizeof(osc_amplitude[0]),
					1, pFile);
			result |= smart_fwrite(&osc_move_period[i],
					sizeof(osc_move_period[0]), 1, pFile);
			result |= smart_fwrite(&osc_location1_p[i],
					sizeof(osc_location1_p[0]), 1, pFile);
			result |= smart_fwrite(&osc_location2_p[i],
					sizeof(osc_location2_p[0]), 1, pFile);
		}

		free(osc_active);
		free (osc_source);
		free(osc_period);
		free(osc_phase);
		free(osc_amplitude);
		free(osc_move_period);
		free(osc_location1_p);
		free(osc_location2_p);

		uint64_t length = description.length();
		result |= smart_fwrite(&length, sizeof(length), 1, pFile);
		result |= smart_fwrite(description.c_str(), 1, length, pFile);

		if (result != OK) {
			cleanOnError(wasWorking, pFile, waveEngine, path_uncompressed);
			return ERROR;
		}

		waveEngine->lock();
		result |= smart_fwrite(waveEngine->getData(Height), sizeof(double_t),
				size * size, pFile);
		result |= smart_fwrite(waveEngine->getData(Velocity), sizeof(double_t),
				size * size, pFile);
		result |= smart_fwrite(waveEngine->getData(Mass), sizeof(double_t),
				size * size, pFile);
		result |= smart_fwrite(waveEngine->getData(Fixity), sizeof(uint8_t),
				size * size, pFile);
		waveEngine->unlock();

		if (result != OK) {
			cleanOnError(wasWorking, pFile, waveEngine, path_uncompressed);
			return ERROR;
		}

		if (wasWorking)
			waveEngine->start();
		fclose(pFile);

		// Compress the file
		int ret;
		pFile = fopen(path_uncompressed.c_str(), "rb");
		FILE * dst = fopen(path.c_str(), "wb");
		if (pFile == nullptr || dst == nullptr)
			return ERROR;SET_BINARY_MODE(pFile);SET_BINARY_MODE(dst);
		ret = def(pFile, dst, Z_BEST_COMPRESSION);
		fclose(pFile);
		fclose(dst);
		remove(path_uncompressed.c_str());
		if (ret != Z_OK) {
			zerr(ret);
			return ERROR;
		}
		return OK;
	}

	return ERROR;
}

ProjectHandlerResult ProjectHandler::openProject(string projectName,
		string & description, WaveEngine * waveEngine) {
	FILE * pFile;
	string path = project_dir + "/" + projectName;
	string path_uncompressed = path + ".uncompressed";

	// Decompress the file
	int ret;
	FILE * src = fopen(path.c_str(), "rb");
	pFile = fopen(path_uncompressed.c_str(), "wb");
	if (src == nullptr || pFile == nullptr)
		return ERROR;SET_BINARY_MODE(src);SET_BINARY_MODE(pFile);
	ret = inf(src, pFile);
	fclose(src);
	fclose(pFile);
	if (ret != Z_OK) {
		zerr(ret);
		return ERROR;
	}

	if ((pFile = fopen(path_uncompressed.c_str(), "rb")) != nullptr) {
		bool wasWorking = waveEngine->isWorking();
		waveEngine->stop();

		uint64_t size;
		double_t loss;
		double_t peak_loss;
		uint64_t absorber_thickness;
		uint8_t shift_to_origin;
		uint8_t enable_absorber;

		uint64_t osc_count;

		// Checkpoints are placed to check for write error.
		int result = OK;
		result |= smart_fread(&size, sizeof(size), 1, pFile);
		result |= smart_fread(&loss, sizeof(loss), 1, pFile);
		result |= smart_fread(&peak_loss, sizeof(peak_loss), 1, pFile);
		result |= smart_fread(&absorber_thickness, sizeof(absorber_thickness),
				1, pFile);
		result |= smart_fread(&shift_to_origin, sizeof(shift_to_origin), 1,
				pFile);
		result |= smart_fread(&enable_absorber, sizeof(enable_absorber), 1,
				pFile);
		waveEngine->setSize(size);
		waveEngine->setLossRatio(loss);
		waveEngine->setAbsorberLossRatio(peak_loss);
		waveEngine->setAbsorberThickness(absorber_thickness);
		waveEngine->setShiftParticlesEnabled(shift_to_origin);
		waveEngine->setAbsorberEnabled(enable_absorber);

		result |= smart_fread(&osc_count, sizeof(osc_count), 1, pFile);

		if (result != OK) {
			cleanOnError(wasWorking, pFile, waveEngine, path_uncompressed);
			return ERROR;
		}

		if (osc_count > MAX_NUMBER_OF_OSCILLATORS) {
			if (wasWorking)
				waveEngine->start();
			fclose(pFile);
			return ERROR;
		}

		uint8_t * osc_active = (uint8_t *) calloc(osc_count, sizeof(uint8_t));
		uint8_t * osc_source = (uint8_t *) calloc(osc_count, sizeof(uint8_t));
		double_t * osc_period = (double_t *) calloc(osc_count,
				sizeof(double_t));
		double_t * osc_phase = (double_t *) calloc(osc_count, sizeof(double_t));
		double_t * osc_amplitude = (double_t *) calloc(osc_count,
				sizeof(double_t));
		double_t * osc_move_period = (double_t *) calloc(osc_count,
				sizeof(double_t));
		Point * osc_location1_p = (Point *) calloc(osc_count, sizeof(Point));
		Point * osc_location2_p = (Point *) calloc(osc_count, sizeof(Point));

		for (uint64_t i = 0; i < osc_count; i++) {
			result |= smart_fread(&osc_active[i], sizeof(osc_active[0]), 1,
					pFile);
			result |= smart_fread(&osc_source[i], sizeof(osc_source[0]), 1,
					pFile);
			result |= smart_fread(&osc_period[i], sizeof(osc_period[0]), 1,
					pFile);
			result |= smart_fread(&osc_phase[i], sizeof(osc_phase[0]), 1,
					pFile);
			result |= smart_fread(&osc_amplitude[i], sizeof(osc_amplitude[0]),
					1, pFile);
			result |= smart_fread(&osc_move_period[i],
					sizeof(osc_move_period[0]), 1, pFile);
			result |= smart_fread(&osc_location1_p[i],
					sizeof(osc_location1_p[0]), 1, pFile);
			result |= smart_fread(&osc_location2_p[i],
					sizeof(osc_location2_p[0]), 1, pFile);

			waveEngine->setOscillatorEnabled(i, osc_active[i]);
			switch (osc_source[i]) {
			case 0:
				waveEngine->setOscillatorSource(i, PointSource);
				break;
			case 1:
				waveEngine->setOscillatorSource(i, LineSource);
				break;
			case 2:
				waveEngine->setOscillatorSource(i, MovingPointSource);
				break;
			}
			waveEngine->setOscillatorPeriod(i, osc_period[i]);
			waveEngine->setOscillatorPhase(i, osc_phase[i]);
			waveEngine->setOscillatorAmplitude(i, osc_amplitude[i]);
			waveEngine->setOscillatorMovePeriod(i, osc_move_period[i]);
			waveEngine->setOscillatorLocation(i, 0, osc_location1_p[i]);
			waveEngine->setOscillatorLocation(i, 1, osc_location2_p[i]);
		}

		free(osc_active);
		free (osc_source);
		free(osc_period);
		free(osc_phase);
		free(osc_amplitude);
		free(osc_move_period);
		free(osc_location1_p);
		free(osc_location2_p);

		uint64_t length = description.length();
		result |= smart_fread(&length, sizeof(length), 1, pFile);
		if (length > 0) {
			char * description_cstring = (char *) calloc(length + 1, 1); // Include null terminator
			result |= smart_fread(description_cstring, 1, length, pFile);
			description = string(description_cstring);
			free(description_cstring);
		} else {
			description = "";
		}

		if (result != OK) {
			cleanOnError(wasWorking, pFile, waveEngine, path_uncompressed);
			return ERROR;
		}

		waveEngine->lock();
		result |= smart_fread(waveEngine->getData(Height), sizeof(double_t),
				size * size, pFile);
		result |= smart_fread(waveEngine->getData(Velocity), sizeof(double_t),
				size * size, pFile);
		result |= smart_fread(waveEngine->getData(Mass), sizeof(double_t),
				size * size, pFile);
		result |= smart_fread(waveEngine->getData(Fixity), sizeof(uint8_t),
				size * size, pFile);
		waveEngine->unlock();

		if (result != OK) {
			cleanOnError(wasWorking, pFile, waveEngine, path_uncompressed);
			return ERROR;
		}

		if (wasWorking)
			waveEngine->start();
		fclose(pFile);
		remove(path_uncompressed.c_str());
		return OK;
	}
	return NOT_FOUND; // This could be due to permissions too.
}

ProjectHandlerResult ProjectHandler::getProjectsList(vector<string> &projects) {
	DIR *pDir;

	struct dirent *pDirEnt;
	if ((pDir = opendir(project_dir.c_str())) == nullptr)
		return ERROR;
	while ((pDirEnt = readdir(pDir)) != NULL) {
		//std::cout << pDirEnt->d_name << ", " << (int)pDirEnt->d_type << std::endl;
		if (pDirEnt->d_type == DT_REG)
			projects.push_back(string(pDirEnt->d_name));
	}
	closedir(pDir);
	return OK;
}

ProjectHandlerResult ProjectHandler::smart_fwrite(const void * data,
		size_t elementSize, size_t numberOfElements, FILE * pFile) {
	size_t writtenElements = 0, lastWritten = 0;
	while (!feof(pFile) && lastWritten >= 0
			&& writtenElements < numberOfElements) {
		lastWritten = fwrite(data, elementSize, numberOfElements, pFile);
		writtenElements += lastWritten;
	}
	if (writtenElements < numberOfElements)
		return ERROR;
	else
		return OK;
}

ProjectHandlerResult ProjectHandler::smart_fread(void * data,
		size_t elementSize, size_t numberOfElements, FILE * pFile) {
	size_t readElements = 0, lastRead = 0;
	while (!feof(pFile) && lastRead >= 0 && readElements < numberOfElements) {
		lastRead = fread(data, elementSize, numberOfElements, pFile);
		readElements += lastRead;
	}
	if (readElements < numberOfElements)
		return ERROR;
	else
		return OK;
}

void ProjectHandler::cleanOnError(bool wasWorking, FILE * pFile,
		WaveEngine * waveEngine, string path_uncompressed) {
	if (wasWorking)
		waveEngine->start();
	fclose(pFile);
	if (path_uncompressed.length() >= 0)
		remove(path_uncompressed.c_str());
}

}
