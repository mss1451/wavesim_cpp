#ifndef WAVEENGINE_H
#define WAVEENGINE_H

#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <iostream>
#include <math.h>
#include <memory.h>
#include <mutex>
#include <sys/sysinfo.h>
#include <thread>
#include <vector>

#define clamp(x, low, high)                                                    \
  (((x) > (high)) ? (high) : (((x) < (low)) ? (low) : (x)))
#define MAX_NUMBER_OF_THREADS 32    // Might also be hard-coded in UI
#define MAX_NUMBER_OF_OSCILLATORS 9 // Might also be hard-coded in UI

// The model described in the article (Spring model). It has a few problems:
// Extremely high frequency waves are hardly absorbed once they are produced.
// Has a potential for positive feedback(PF). The PF usually results in chaos.
// The chaos makes the system unusable; very powerful noise takes place.
// The other reasons, if any, for the chaos is unknown.
// Increase the mass of particles for a lower chance of chaos.
#define WAVE_ENGINE_SPRING_MODEL

// WAVE_ENGINE_X_MODEL definition was for an attempt of creating an alternative
// model for the engine. All the other models failed so far.

namespace WaveSimulation {

class WaveEngine;

struct ParticleLocationInfo {
public:
  bool top, left, right, bottom;
};

enum ParticleAttribute {
  Height = 1,
  Velocity = 2,
  Loss = 4,
  Mass = 8,
  Fixity = 16
};

enum coThreadMission {
  Pause = 1,
  Destroy = 2,
  CalculateForces = 4,
  CalculateColors = 8
};

enum OscillatorSource { PointSource, LineSource, MovingPointSource };

struct Color {
public:
  uint8_t r, g, b;

  Color(uint8_t r, uint8_t g, uint8_t b) {
    this->r = r;
    this->g = g;
    this->b = b;
  }

  Color(uint32_t rgb32) {
    r = rgb32;
    g = rgb32 >> 8;
    b = rgb32 >> 16;
  }

  uint32_t ToRGB32() { return r + (g << 8) + (b << 16); }

  Color() {
    this->r = 0;
    this->g = 0;
    this->b = 0;
  }
};

struct Point {
public:
  double x, y;

  Point(double x, double y) {
    this->x = x;
    this->y = y;
  }

  Point() {
    this->x = 0;
    this->y = 0;
  }

  static double dot(Point p1, Point p2) { return p1.x * p2.x + p1.y * p2.y; }

  static double dist(Point p1, Point p2) {
    return sqrt(pow(p1.x - p2.x, 2) + pow(p1.y - p2.y, 2));
  }

  Point operator+(Point p) { return Point(this->x + p.x, this->y + p.y); }
  Point operator-(Point p) { return Point(this->x - p.x, this->y - p.y); }

  Point operator*(double d) { return Point(this->x * d, this->y * d); }

  friend Point operator*(double lhs, Point rhs) {
    return Point(lhs * rhs.x, lhs * rhs.y);
  }
};

struct Rectangle {
public:
  double x, y, width, height;

  Rectangle(double x, double y, double width, double height) {
    this->x = x;
    this->y = y;
    this->width = width;
    this->height = height;
  }

  Rectangle() {
    this->x = 0;
    this->y = 0;
    this->width = 0;
    this->height = 0;
  }
};

struct coThreadStruct {
public:
  int tIndex, firstIndex, count;
  bool busy = false;
  coThreadStruct(int xtIndex, int xfirstIndex, int xcount) {
    tIndex = xtIndex;
    firstIndex = xfirstIndex;
    count = xcount;
  }

  coThreadStruct() {
    tIndex = 0;
    firstIndex = 0;
    count = 0;
  }
};

struct coThreadFuncArg {
public:
  WaveEngine *waveEngine;
  int index;

  coThreadFuncArg(WaveEngine *waveEngine, int index) {
    this->index = index;
    this->waveEngine = waveEngine;
  }

  coThreadFuncArg() {
    this->index = 0;
    this->waveEngine = NULL;
  }
};

class WaveEngine {
protected:
  typedef void (*RenderCallback)(uint8_t *bitmap_data,
                                 unsigned long data_length, void *extra_data);

  // pd -> particle data

  double_t *pd = 0;            // Height map
  double_t *pd_previous = 0;   // Previous height map for calculations
  double_t average_height = 0; // Average height of the height map for shifting
  double_t *pdv = 0;           // Velocity map
  double_t *pdl = 0;           // Loss map
  double_t *pdm = 0;           // Mass map
  double_t loss = 0.0; // Reduces mechanical energy (potential + kinetic) by the
                       // (100 * loss) percent.
  uint8_t *pd_static = 0; // Static particle map. Particles which will act like
                          // a obstacle or wall.
  ParticleLocationInfo *pd_location_info =
      0; // Location cache to speed up calculations.

  bool *osc_active;             // Oscillator turned on/off?
  OscillatorSource *osc_source; // Whether to emit from a point or line, or move
                                // it from location-1 to location-2.
  double *osc_period;           // Period of oscillator in iterations per cycle.
  double *osc_phase;            // Phase of oscillator in radians.
  double *osc_amplitude;        // Amplitude of oscillator.
  double *osc_move_period; // Move period of oscillator in iterations per cycle.
  unsigned int **osc_locations;     // Oscillator location set.
  unsigned int *osc_locations_size; // For each oscillator, how many index
                                    // values are there?
  Point *osc_location1_p; // Cache location in different types to avoid frequent
                          // casting.
  Point *osc_location2_p;

  double FPS = 25;  // Frames per second limiter. No limit if zero but might be
                    // very expensive so not recommended.
  double IPS = 100; // Iterations per second limiter. No limit if zero but
                    // results in CPU overuse and inconsistent speed.
  unsigned int calcCounter = 0; // Global calculation counter for oscillators.
  unsigned int calcDone = 0;    // How many calculations have been done so far?
  unsigned int paintDone = 0;   // How many paintings have been done so far?
  unsigned int numOfThreads =
      1; // Number of co-threads. Should be equal to the number of CPU cores for
         // the best performance.
  unsigned int TDelay = 5; // Sleep delay for the Main Thread at pause in ms.
  std::thread MainT; // Main thread that will generate and run 'numOfThreads'
                     // co-threads.
  std::thread *coThreads; // The engine will benefit from all of the cores of
                          // CPU efficiently with these co-threads.
  std::mutex mutex;       // For properties and general access restrictions.
  std::mutex *mCT;        // Co-thread mutex
  std::condition_variable *mCTC;     // Co-thread condition
  std::mutex mCout;                  // Prevent simultaneous calls to std::cout
  coThreadMission ctMission = Pause; // Defines the mission for each co-thread.
  coThreadStruct *
      ctStruct; // Data argument for co-threads that define their working range.
  bool renderEnabled =
      true; // False = Halt render callback including the painting calculations.
  bool calculationEnabled = true; // False = Halt calculations.

  bool locked = false; // Is the pool locked externally for data access?

  bool logPerformance = true; // Log iteration and paint performances.
  unsigned int performanceLogInterval = 1000; // milliseconds

  // Don't stop working but put the threads on sleep more frequently to prevent
  // extreme CPU usage. Decreases performance and accuracy of limiters
  // especially at high iterations-per-second values.
  bool powerSaveMode = false;

  RenderCallback renderCallback = NULL; // Render callback function.

  uint8_t *bitmap_data =
      0;            // Color data that is passed to the callback function.
  void *extra_data; // Extra data that is passed to the callback function (such
                    // as canvas class).

  bool extremeContrastEnabled =
      false; // There will be only three colors if true: (A+B)/2 for natural, A
             // for crest, and B for trough.
  unsigned int amplitudeMultiplier =
      20; // Multiplied by height at painting stage to reveal weaker vibrations.

  Color crestColor = Color(255, 255, 255);
  Color troughColor = Color(0, 0, 0);
  Color staticColor = Color(255, 255, 0);

  bool massMap = false;           // Display mass map.
  double massMapRangeHigh = 50.0; // Maximum value for coloring of mass regions.
  double massMapRangeLow = 1.0;   // Minimum value for coloring of mass regions.

  bool work_now = false; // True = Thread must make calculations now, False =
                         // Thread must sleep now.

  bool shifting = true; // True = Shift average height/alpha to origin(zero).

  bool disposing = false; // It will be true once the termination starts.

  unsigned int size = 300; // Size of the wave pool. It indicates both the width
                           // and height since the pool will always be a square.
  unsigned int sizesize = size * size; // Just for further optimization
  double sizesized = size * size;      // Just for further optimization

  // These variables are used for absorber. It is used for eliminating
  // reflection from window boundaries.

  unsigned int absorb_offset =
      25; // Offset from each window boundary where the loss starts to increase.
  double max_loss =
      0.3; // The highest loss value. They are located at the boundaries.
  bool absorberEnabled =
      true; // If true, the particles near the boundaries will have high loss.

public:
  double getLossRatio();
  void setLossRatio(double loss);

  bool getOscillatorEnabled(unsigned int oscillatorId);
  void setOscillatorEnabled(unsigned int oscillatorId, bool enabled);

  OscillatorSource getOscillatorSource(unsigned int oscillatorId);
  void setOscillatorSource(unsigned int oscillatorId,
                           OscillatorSource oscillatorSource);

  double getOscillatorPeriod(unsigned int oscillatorId);
  void setOscillatorPeriod(unsigned int oscillatorId, double period);

  double getOscillatorPhase(unsigned int oscillatorId);
  void setOscillatorPhase(unsigned int oscillatorId, double phase);

  double getOscillatorAmplitude(unsigned int oscillatorId);
  void setOscillatorAmplitude(unsigned int oscillatorId, double amplitude);

  double getOscillatorMovePeriod(unsigned int oscillatorId);
  void setOscillatorMovePeriod(unsigned int oscillatorId, double movePeriod);

  Point getOscillatorLocation(unsigned int oscillatorId,
                              unsigned int locationId);
  void setOscillatorLocation(unsigned int oscillatorId, unsigned int locationId,
                             Point location);

  // Where exactly is the oscillator now if it is moving?
  Point getOscillatorRealLocation(unsigned int oscillatorId);

  double getFramesPerSecond();
  void setFramesPerSecond(double framesPerSecond);

  double getIterationsPerSecond();
  void setIterationsPerSecond(double iterationsPerSecond);

  unsigned int getNumberOfThreads();
  void setNumberOfThreads(unsigned int numberOfThreads);

  unsigned int getThreadDelay();
  void setThreadDelay(unsigned int threadDelay);

  bool getRenderEnabled();
  void setRenderEnabled(bool);

  bool getCalculationEnabled();
  void setCalculationEnabled(bool);

  bool getLogPerformance();
  void setLogPerformance(bool);

  bool getPowerSaveMode();
  void setPowerSaveMode(bool);

  unsigned int getPerformanceLogInterval();
  void setPerformanceLogInterval(unsigned int);

  double getMassMapRangeHigh();
  void setMassMapRangeHigh(double massMapRangeHigh);

  double getMassMapRangeLow();
  void setMassMapRangeLow(double massMapRangeLow);

  bool getShowMassMap();
  void setShowMassMap(bool showMassMap);

  unsigned int getSize();
  void setSize(unsigned int size);

  double getAbsorberLossRatio();
  void setAbsorberLossRatio(double absorberLoss);

  unsigned int getAbsorberThickness();
  void setAbsorberThickness(unsigned int absorberThickness);

  bool getShiftParticlesEnabled();
  void setShiftParticlesEnabled(bool shiftParticles);

  bool getAbsorberEnabled();
  void setAbsorberEnabled(bool absorberEnabled);

  RenderCallback getRenderCallback();
  void setRenderCallback(RenderCallback renderCallback, void *extra_data);

  void *getExtraData();

  bool getExtremeContrastEnabled();
  void setExtremeContrastEnabled(bool extremeContrastEnabled);

  unsigned int getAmplitudeMultiplier();
  void setAmplitudeMultiplier(unsigned int amplitudeMultiplier);

  Color getCrestColor();
  void setCrestColor(Color crestColor);

  Color getTroughColor();
  void setTroughColor(Color troughColor);

  Color getStaticColor();
  void setStaticColor(Color staticColor);

  // Initializes the WaveEngine
  WaveEngine();
  virtual ~WaveEngine();

  // Locks the entire pool and allows the access to particle data.\n
  // Returns false if already locked.\n\n
  // Note that this stops the calculations until the unlock is called.\n
  // Not any function must be called except 'getData' and 'unlock' otherwise the
  // process will hang.
  bool lock();

  // Unlocks the entire pool and resumes any previously ongoing calculations.
  bool unlock();

  // Gets the pointer to the data array corresponding to the specified particle
  // attribute.\n Call this method between lock() and unlock() methods to obtain
  // consistent data.\n Do not specify multiple attributes.\n Returned data type
  // is 'double' for everything except 'static' which is of 'bool' type.
  void *getData(ParticleAttribute particleAttribute);

  // Starts the force calculation.
  void start();

  /// Suspends the force calculation indefinitely.
  void stop();

  bool isWorking();

protected:
  static void CoThreadFunc(void *data);

  static void MainThreadFunc(void *data);

  void sendOrderToCT(coThreadMission order);

  void waitForCT();

  bool calculateForces(const unsigned int firstIndex, const unsigned int count);

  bool paintBitmap(const unsigned int firstIndex, const unsigned int count,
                   uint8_t *);

  void setCoThreads(unsigned int oldNumOfThreads);

  void setLossRatio();

  void setPool(unsigned int oldsize);

  void updateOscLocIndices(unsigned int oscillatorId);

  void mutexedCout(const char *msg);

  // Misc
  template <typename T> int sgn(T val) { return (T(0) < val) - (val < T(0)); }
};
} // namespace WaveSimulation

#endif /* WAVEENGINE_H */
