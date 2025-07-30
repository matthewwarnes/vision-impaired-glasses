# vision-impaired-glasses


## Dependencies

Before compiling and running this application the following dependencies must be installed

* cmake
* build-essential
* libasound2-dev
* libssl-dev
* libopencv-dev

```sudo apt-get install build-essential cmake libasound2-dev libssl-dev libopencv-dev```

## Download and Compiling

To download the git repo, clone with the following command:

```git clone https://github.com/matthewwarnes/vision-impaired-glasses```

To compile the application run the following script from the repository:

```./build.sh```

## Packaging and Production Installation

To build the application and associated data files into an easy to install package run the following script:

```./package.sh```

This will produce a debian package under the bin directory which can be installed using dpkg tool.

## Configuration

The application uses a yaml configuration file to control the application options.

An example config.yaml file is included in the root of the repository, this has been commented to explain the options.

When packaged into a debian package for system installation, the package/config.yaml file is included in the installation, and this file can be edited in the repo to keep a valid configuration for installation.

## Preparing Speech Models

Local speech detection is used in the software to detect the presence of speech and perform at least activation work detection.

For this to work binary files are requried which contain the speech models. These can be downloaded by running the following script:

```./downloadWhisperModels.sh```

This will generate two binary files in the local directory (ggml-base.en.bin and ggml-silero-v5.1.2.bin).
For installed applications these are currently expected to be copied to "/home/orangepi/"

## Openai Api Key

For integration with openai apis, a key is required.

This key can be optained from the openai website when creating an account

This should be stored in a file, and the application will read it from the configured file.
The installed application configuration assumes this is at "/home/orangepi/oai_key"

## Running Application

To run the compiled application from the repository run the following command from the root of the repo:

```./bin/glasses -c config.yaml```

This will start the application and use the config.yaml file in the root of the repo.


Once the application has been installed using the debian package it can be run from anywhere as follows:

```glasses -c <full_path_to_config>```


## Running on Startup

Work in progress

## Source Code Layout

The C++ source code is located under the "src" directory.

The "control_thread.cpp" file is a class which runs as a seperate thread for handling voice recognition, speech playback and AI integration.

The "image_thread.cpp" file is the start of a class to handle opencv image processing and display purposes, which also runs as a separate thread (you can essentially think of the "thread_handler" function as a main function for this thread). This basic implementation includes support for the control thread to send messages to the image thread for control as well as request current frame for sending to AI requests.
