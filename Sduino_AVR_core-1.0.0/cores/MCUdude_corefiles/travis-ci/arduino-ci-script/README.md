arduino-ci-script
==========

Bash script for continuous integration of [Arduino](http://www.arduino.cc/) projects. I'm using this centrally managed script for multiple repositories to make updates easy. I'm using this with [Travis CI](http://travis-ci.org/) but it could be easily adapted to other purposes.

[![Build Status](https://travis-ci.org/per1234/arduino-ci-script.svg?branch=master)](https://travis-ci.org/per1234/arduino-ci-script)

#### Installation
- You can download a .zip of all the files from https://github.com/per1234/arduino-ci-script/archive/master.zip
- Include the script in your project by adding the following line:
```bash
  - source arduino-ci-script.sh
```
- It's possible to leave the script hosted at this repository.
```bash
  - source <(curl -SLs https://raw.githubusercontent.com/per1234/arduino-ci-script/master/arduino-ci-script.sh)
```
- The above doesn't allow you control over the version and would not be a good idea for security reasons if you use the functions that take a GitHub token argument. You could use the script at a specific point in the commit history with something like this:
```bash
  - git clone https://github.com/per1234/arduino-ci-script.git "${HOME}/arduino-ci-script"
  - cd "${HOME}/arduino-ci-script"
  - git checkout 0355314d45970cd33e52ebba9967646a063ea9eb
  - source "${HOME}/arduino-ci-script/arduino-ci-script.sh"
```


#### Usage
See https://github.com/per1234/WatchdogLog/blob/master/.travis.yml for an example of the script in use. Please configure your continuous integration system to make the minimum number of downloads and sketch verifications necessary to effectively test your code. This will prevent wasting Arduino and Travis CI's bandwidth while making the builds run fast.
##### `set_script_verbosity SCRIPT_VERBOSITY_LEVEL`
Control the level of verbosity of the script's output in the Travis CI log. Verbose output can be helpful for debugging but in normal usage it makes the log hard to read and may cause the log to exceed Travis CI's maximum log size of 4 MB, which causes the job to be terminated. The default verbosity level is `0`.
- Parameter: **SCRIPT_VERBOSITY_LEVEL** - `0`, `1` or `2` (least to most verbosity).

##### `set_application_folder APPLICATION_FOLDER`
- Parameter: **APPLICATION_FOLDER** - The folder to install the Arduino IDE to. This should be set to `/usr/local/share` or a subfolder of that location. The folder will be created if it doesn't already exist. The Arduino IDE will be installed in the `arduino` subfolder.

##### `set_sketchbook_folder SKETCHBOOK_FOLDER`
- Parameter: **SKETCHBOOK_FOLDER** - The folder to be set as the Arduino IDE's sketchbook folder. The folder will be created if it doesn't already exist. Libraries installed via `install_library` will be installed to the `libraries` subfolder. Non-Boards Manager hardware packages installed via `install_package` will be installed to the `hardware` subfolder. This setting is only supported by Arduino IDE 1.5.6 and newer.

##### `set_board_testing BOARD_TESTING`
Turn on/off checking for errors with the board definition that don't affect sketch verification such as missing bootloader file. If this is turned on and an error is detected the build will be failed. This feature is off by default.
- Parameter: **BOARD_TESTING** - `true`/`false`

##### `set_library_testing LIBRARY_TESTING`
Turn on/off checking for errors with libraries that don't affect sketch verification such as missing or invalid items in the library.properties file. If this is turned on and an error is detected the build will be failed. This feature is off by default.
- Parameter: **LIBRARY_TESTING** - `true`/`false`

##### Special version names:
  - `all`: Refers to all versions of the Arduino IDE (including the hourly build). In the context of `install_ide` this means all IDE versions listed in the script (those that support the command line interface, 1.5.2 and newer). In the context of all other functions this means all IDE versions that were installed via `install_ide`.
  - `oldest`: The oldest release version of the Arduino IDE. In the context of `install_ide` this is the oldest of the IDE versions listed in the script (1.5.2, the first version to have a command line interface). In the context of build_sketch this means the oldest IDE version that was installed via `install_ide`.
  - `newest`: Refers to the newest release version of the Arduino IDE (not including the hourly build unless hourly is the only version on the list). In the context of `install_ide` this means the newest IDE version listed in the script. In the context of all other functions this means the newest IDE version that was installed via `install_ide`.
  - `hourly`: The hourly build of the Arduino IDE. Note that this IDE version is intended for beta testing only.

##### `install_ide [IDEversionList]`
Install a list of version(s) of the Arduino IDE.
- Parameter(optional): **IDEversionList** - A list of the versions of the Arduino IDE you want installed, in order from oldest to newest. e.g. `'("1.6.5-r5" "1.6.9" "1.8.2")'`. If no arguments are supplied all IDE versions will be installed. I have defined all versions of the Arduino IDE that have a command line interface in the script for the sake of being complete but I really don't see much reason for testing with the 1.5.x versions of the Arduino IDE. Please only install the IDE versions you actually need for your test to avoid wasting Arduino's bandwidth. This will also result in the builds running faster.

##### `install_ide startIDEversion [endIDEversion]`
Install a range of version(s) of the Arduino IDE.
- Parameter: **startIDEversion** - The oldest version of the Arduino IDE to install.
- Parameter(optional): **endIDEversion** - The newest version of the Arduino IDE to install. If this argument is omitted then only startIDEversion will be installed.

##### `install_package`
"Manually" install the hardware package from the current repository. Packages are installed to `$SKETCHBOOK_FOLDER/hardware. Assumes the hardware package is located in the root of the download or repository and has the correct folder structure.

##### `install_package packageURL`
"Manually" install a hardware package downloaded as a compressed file. Packages are installed to `$SKETCHBOOK_FOLDER/hardware. Assumes the hardware package is located in the root of the file and has the correct folder structure.
- Parameter: **packageURL** - The URL of the hardware package download. The protocol component of the URL (e.g. `http://`, `https://`) is required.

##### `install_package packageURL [branchName]`
"Manually" install a hardware package. Packages are installed to `$SKETCHBOOK_FOLDER/hardware. Assumes the hardware package is located in the root of the repository and has the correct folder structure.
- Parameter: **packageURL** - The URL of the Git repository. The protocol component of the URL (e.g. `http://`, `https://`) is required.
- Parameter(optional): **branchName** - Branch of the repository to install. If this argument is not specified or is left blank the default branch will be used.

##### `install_package packageID [packageURL]`
Install a hardware package using the Arduino IDE (Boards Manager). Only the **Arduino AVR Boards** package is included with the Arduino IDE installation. Packages are installed to `$HOME/.arduino15/packages. You must call `install_ide` before this function. This feature is only available with Arduino IDE 1.6.4 and newer.
- Parameter: **packageID** - `package name:platform architecture[:version]`. If `version` is omitted the most recent version will be installed. e.g. `arduino:samd` will install the most recent version of **Arduino SAM Boards**.
- Parameter(optional): **packageURL** - The URL of the Boards Manager JSON file for 3rd party hardware packages. This can be omitted for hardware packages that are included in the official Arduino JSON file (e.g. Arduino SAM Boards, Arduino SAMD Boards, Intel Curie Boards).

##### `install_library`
Install the library from the current repository. Assumes the library is in the root of the repository. The library is installed to the `libraries` subfolder of the sketchbook folder.

##### `install_library libraryName`
Install a library that is listed in the Arduino Library Manager index. The library is installed to the `libraries` subfolder of the sketchbook folder. You must call `install_ide` before this function. This feature is only available with Arduino IDE 1.6.4 and newer installed.
- Parameter: **libraryName** - The name of the library to install. You can specify a version separated from the name by a colon, e.g. "LiquidCrystal I2C:1.1.2". If no version is specified the most recent version will be installed. You can also specify comma-separated lists of library names.

##### `install_library libraryURL [newFolderName]`
Download a library in a compressed file from a URL. The library is installed to the `libraries` subfolder of the sketchbook folder.
- Parameter: **libraryURL** - The URL of the library download or library name in the Arduino Library Manager. The protocol component of the URL (e.g. `http://`, `https://`) is required. This can be any compressed file format. Assumes the library is located in the root of the file.
- Parameter(optional): **newFolderName** - Folder name to rename the installed library folder to. This can be useful if the default folder name of the downloaded file is problematic. The Arduino IDE gives include file preference when the filename matches the library folder name. GitHub's "Download ZIP" file is given the folder name {repository name}-{branch name}. Library folder names that contain `-` or `.` are not compatible with Arduino IDE 1.5.6 and older, arduino will hang if it's started with a library using an invalid folder name installed.

##### `install_library libraryURL [branchName [newFolderName]]`
Install a library by cloning a Git repository). The library is installed to the `libraries` subfolder of the sketchbook folder.
- Parameter: **libraryURL** - The URL of the library download or library name in the Arduino Library Manager. The protocol component of the URL (e.g. `http://`, `https://`) is required. Assumes the library is located in the root of the repository.
- Parameter(optional): **branchName** - Branch of the repository to install. If this argument is not specified or is left blank the default branch will be used.
- Parameter(optional): **newFolderName** - Folder name to rename the installed library folder to. This can be useful if the default folder name of the downloaded file is problematic. The Arduino IDE gives include file preference when the filename matches the library folder name. Library folder names that contain `-` or `.` are not compatible with Arduino IDE 1.5.6 and older, arduino will hang if it's started with a library using an invalid folder name installed. If the `newFolderName` argument is specified the `branchName` argument must also be specified. If you don't want to specify a branch then use `""` for the `branchName` argument.

##### `set_verbose_output_during_compilation verboseOutputDuringCompilation`
Turn on/off arduino verbose output during compilation. This will show all the commands arduino runs during the process rather than just the compiler output. This is usually not very useful output and only clutters up the log. This feature is off by default.
- Parameter: **verboseOutputDuringCompilation** - `true`/`false`

##### `build_sketch sketchPath boardID allowFail IDEversion`
##### `build_sketch sketchPath boardID allowFail [IDEversionList]`
##### `build_sketch sketchPath boardID allowFail startIDEversion endIDEversion`
Pass some parameters from .travis.yml to the script. `build_sketch` will echo the arduino exit status to the log, which is documented at https://github.com/arduino/Arduino/blob/master/build/shared/manpage.adoc#exit-status.
- Parameter: **sketchPath** - Path to a sketch or folder containing sketches. If a folder is specified it will be recursively searched and all sketches will be verified.
- Parameter: **boardID** - `package:arch:board[:parameters]` ID of the board to be compiled for. e.g. `arduino:avr:uno`. Board-specific parameters are only supported by Arduino IDE 1.5.5 and newer.
- Parameter: **allowFail** - `true`, `require`, or `false`. Allow the verification to fail without causing the CI build to fail. `require` will cause the build to fail if the sketch verification doesn't fail.
- Parameter: **IDEversion** - A single version of the Arduino IDE to use to verify the sketch.
- Parameter(optional): **IDEversionList** - A list of versions of the Arduino IDE to use to verify the sketch. e.g. `'("1.6.5-r5" "1.6.9" "1.8.2")'`. If no version list is provided all installed IDE versions will be used.
- Parameter: **startIDEversion** - The start (inclusive) of a range of versions of the Arduino IDE to use to verify the sketch.
- Parameter: **endIDEversion** - The end (inclusive) of a range of versions of the Arduino IDE to use to verify the sketch.

##### `display_report`
Echo a tab separated report of all verification results to the log. The report is located in `${HOME}/arduino-ci-script_report`. Note that Travis CI runs each build of the job in a separate virtual machine so if you have multiple jobs you will have multiple reports. The only way I have found to generate a single report for all tests is to run them as a single job. This means not setting multiple matrix environment variables in the `env` array. See https://docs.travis-ci.com/user/environment-variables. The report consists of:
- Build timestamp
- Build - The Travis CI build number.
- Job - Travis CI job number
- Job URL - The URL of the Travis CI job log.
- Build Trigger - The cause of this Travis CI build. Values are `push`, `pull_request`, `api`, `cron`.
- Allow Job Failure - Whether the Travis CI configuration was set to allow the failure of this job without failing the build.
- PR# - Pull request number (if build was triggered by a pull request).
- Branch - The branch of the repository that was built.
- Commit - Commit hash of the build.
- Commit range - The range of commits that were included in the push or pull request.
- Commit Message - First line of the commit message
- Sketch filename
- Board ID
- IDE version
- Program Storage (bytes) - Program storage usage of the compiled sketch.
- Dynamic Memory (bytes) - Dynamic memory usage by global variables in the compiled sketch (not available for some boards).
- # Warnings - Number of warnings reported by the compiler during the sketch compilation.
- Allow Failure - Whether the sketch verification was allowed to fail (set by the `allowFail` argument of `build_sketch`).
- Exit Status - Exit status returned by arduino after the sketch verification.
- # Board Issues - The number of board issues detected.
- Board Issue - Short description of the last board issue detected.
- # Library Issues - The number of library issues detected. Library issues are things that cause warnings in the sketch verification output from the IDE, rather than the compiler.
- Library Issue - Short description of the last library issue detected.

##### `publish_report_to_repository REPORT_GITHUB_TOKEN repositoryURL reportBranch reportFolder doLinkComment`
Add the report to a repository. See the [instructions for publishing job reports](publishing-job-reports) for details.
- Parameter: **REPORT_GITHUB_TOKEN** - The hidden or encrypted environment variable containing the GitHub personal access token.
- Parameter: **repositoryURL** - The .git URL of the repository to publish the report to. This URL can be found by clicking the "Clone or download" button on the home page of the repository. The repository must already exist.
- Parameter: **reportBranch** - The branch to publish the report to. The branch must already exist.
- Parameter: **reportFolder** - The the folder to publish the report to. The folder will be created if it doesn't exist.
- Parameter: **doLinkComment** - `true` or `false` Whether to comment on the commit with a link to the report.

##### `publish_report_to_gist REPORT_GITHUB_TOKEN REPORT_GIST_URL doLinkComment`
Add the report to the report gist. See the [instructions for publishing job reports](publishing-job-reports) for details.
- Parameter: **REPORT_GITHUB_TOKEN** - The hidden or encrypted environment variable containing the GitHub personal access token.
- Parameter: **REPORT_GIST_URL** - The URL of the report gist.
- Parameter: **doLinkComment** - `true` or `false` Whether to comment on the commit with a link to the report.


#### Publishing job reports
The script offers the option of publishing the job result reports to a repository or GitHub [gist](https://gist.github.com/) by using the `publish_report_to_repository` or `publish_report_to_gist` functions. This makes it easier to view the reports or to import them into a spreadsheet program. You also have the option of having the link to the reports automatically added in a comment to the commit being tested. This requires some configuration, which is described in the instructions below.

NOTE: For security reasons reports for builds of pull requests from a fork of the repository can not be published. If the owner of that fork wants to publish reports they can create a GitHub token (and gist if using `publish_report_to_gist`) and configure the Travis CI settings for their fork of the repository following these instructions.
##### Creating a GitHub personal access token
This is required for either publishing option.
1. Sign in to your GitHub account.
2. Click your avatar at the top right corner of GitHub > **Settings** > **Personal access tokens** > **Generate new token**.
3. Check the appropriate permissions for the token:
  1. If using `publish_report_to_gist` check **gist**.
  2. If using `publish_report_to_repository` or setting the `doLinkComment` argument of `publish_report_to_gist` check **public_repo** (for public repositories only) or **repo** (for private and public repositories).
5. Click the **Generate token** button.
6. When the generated token is displayed click the clipboard icon next to it to copy the token.
7. Open the settings page for your repository on the Travis CI website.
8. In the **Environment Variables** section enter `REPORT_GITHUB_TOKEN` in the **Name** field and the token in the **Value** field. Make sure the **Display value in build log** switch is in the off position.
9. Click the **Add** button.

An alternative to using a Travis CI hidden environment variable as described above is to define the GitHub personal access token as an encrypted environment variable: https://docs.travis-ci.com/user/environment-variables/#Encrypting-environment-variables.
##### Creating a gist
This is required for use of the `publish_report_to_gist` function.
1. Open https://gist.github.com/
2. Sign in to your GitHub account.
3. Type an appropriate name in the **Filename including extension...** field. Gists sort files alphabetically so the filename should be something that will sort before the report filenames, which start at travis_ci_job_report_1.1.tsv.
4. Add some text to the file contents box.
5. Click **Create secret gist** if you don't want the gist to be discoverable (it can still be read by anyone who knows the URL), or **Create public gist** to make it discoverable.
6. Copy the URL of the gist.
7. Open the settings page for your repository on the Travis CI website.
8. In the **Environment Variables** section enter `REPORT_GIST_URL` in the **Name** field and the URL of the gist in the **Value** field. You can turn the **Display value in build log** switch to the on position. The gist URL is not secret and this will provide more information in the log.
9. Click the **Add** button.


#### Troubleshooting
##### Script hangs after an arduino command
The Arduino IDE will usually try to start the GUI whenever there is an error in the command. Since the Travis CI build environment does not support this it will just hang for ten minutes until Travis CI automatically cancels the job. This means you get no useful information on the cause of the problem.
##### Verbose output
Verbose output results in a harder to read log so you should leave it off or minimized when possible. Note that turning on verbose output for a large build may cause the log to exceed 4 MB, which causes Travis CI to terminate the job.
- Verbose script output - Add or uncomment the following lines in your `.travis.yml` file to get more information for troubleshooting.
  - Print shell input lines as they are read:
    - `- set_verbose_script_output "true"`
  - Print a trace of simple commands, for commands, case commands, select commands, and arithmetic for commands and their arguments or associated word lists after they are expanded and before they are executed. The value of the PS4 variable is expanded and the resultant value is printed before the command and its expanded arguments.
    - `- set_more_verbose_script_output "true"`
- Verbose output for Travis CI and script - Add one or both of the following lines to your `.travis.yml` file to get more information for troubleshooting of both the Travis CI build process and the script. Do not turn on verbosity by passing `true` to `set_verbose_script_output` or `set_more_verbose_script_output` when you have these lines in your `.travis.yml` file.
  - Print shell input lines as they are read:
    - `- set -o verbose`
  - Print a trace of simple commands, for commands, case commands, select commands, and arithmetic for commands and their arguments or associated word lists after they are expanded and before they are executed. The value of the PS4 variable is expanded and the resultant value is printed before the command and its expanded arguments.
    - `- set -o xtrace`
- Verbose output during compilation - Add the following line to your `.travis.yml` file to get verbose output from arduino of the commands used in the sketch building process:
  - `set_verbose_output_during_compilation true`
##### Problematic IDE versions
Some older versions of the Arduino IDE have bugs or limitations that may cause problems if used with this script:
- 1.5.1 and older - The command line interface was added in 1.5.2, thus no version older than that can be used.
- 1.5.4 and older - Do not support board-specific parameters, set by custom **Tools** menu items.
- 1.5.5 and older - Do not support setting preferences (`--pref`), thus the sketchbook folder argument of `set_parameters` will not be used.
- 1.5.5-r2 and older - Don't recognize libraries that have a library.properties` file that doesn't define a `core-dependencies` property. The file include is successful but compilation of sketches that use the library functions will fail.
- 1.5.6 and older - `-` or `.` are not allowed in sketch or library folder names. If any are present the Arduino IDE will hang indefinitely when it's executed.
- 1.6.2 - Moves its hardware packages to the .arduino15 folder, causing all other IDE versions to use those cores, some of which are not compatible. For this reason 1.6.2 has been removed from the default list of versions but may still be specified via the `IDE_VERSIONS` argument.
- 1.6.3 and older - Do not support installing boards (`--install-boards`), thus `install_package` can't be used.

#### Contributing
Pull requests or issue reports are welcome! Please see the [contribution rules](https://github.com/per1234/arduino-ci-script/blob/master/CONTRIBUTING.md) for instructions.
