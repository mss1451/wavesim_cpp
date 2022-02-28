#ifndef HEADER_PROJECT_HANDLER_H_
#define HEADER_PROJECT_HANDLER_H_

#include <dirent.h>
#include <iostream>
#include <sys/stat.h>
#include <sys/types.h>
#include <vector>
extern "C" {
#include "zutil.h"
}
#include "wave_engine.h"

using namespace std;

namespace WaveSimulation {

enum ProjectHandlerResult { OK, NOT_FOUND, ERROR };

// Takes a pointer to a WaveEngine object and saves it to or modifies it from a
// project file.
class ProjectHandler {
protected:
  string project_dir = "./data/scenes";

public:
  // project_folder: Absolute path to the projects folder.
  ProjectHandler(string project_folder, bool create_dir);
  ProjectHandler();
  virtual ~ProjectHandler();

  // Get the projects path. The path returned is absolute.
  string getProjectsPath();

  // Set the projects path. The path must be absolute.\n
  // create: Create if folder doesn't exist.
  ProjectHandlerResult setProjectsPath(string path, bool create);

  // Opens a wave simulator project.\n
  // Path must not be included as all projects are found in a certain folder.\n
  // waveEngine: WaveEngine instance to load the data to.\n\n
  ProjectHandlerResult openProject(string projectName, string &description,
                                   WaveEngine *waveEngine);

  // Saves a wave simulator project.\n
  // Path must not be included as all projects are found in a certain folder.\n
  // If a project with the same name exists, it is overwritten.
  // To handle this, check the existence of the project first.\n
  // waveEngine: WaveEngine instance to save the data of.\n\n
  ProjectHandlerResult saveProject(string projectName, string description,
                                   WaveEngine *waveEngine);

  // Deletes a wave simulator project.\n
  // Path must not be included as all projects are found in a certain folder.\n
  ProjectHandlerResult deleteProject(string projectName);

  // Returns a string array containing the list of project names found under the
  // project folder.\n Returns null if not found any projects or there is an
  // error.
  ProjectHandlerResult getProjectsList(vector<string> &projects);

  // Misc
  ProjectHandlerResult smart_fwrite(const void *data, size_t elementSize,
                                    size_t numberOfElements, FILE *pFile);

  ProjectHandlerResult smart_fread(void *data, size_t elementSize,
                                   size_t numberOfElements, FILE *pFile);

  void cleanOnError(bool wasWorking, FILE *pFile, WaveEngine *waveEngine,
                    string path_uncompressed);
};
} // namespace WaveSimulation

#endif /* HEADER_PROJECT_HANDLER_H_ */
