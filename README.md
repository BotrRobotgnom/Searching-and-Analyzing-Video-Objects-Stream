# Searching and Analyzing Unique Objects in a Video Stream

A desktop application for frame-by-frame video inspection, manual object annotation, and simple neural-network-based object analysis.

This repository contains a C++/Win32 prototype developed as part of a bachelor's thesis project. The application combines classic video processing with an interactive labeling workflow: a user can load a video, move through individual frames, mark object regions on a grid, save masks, train a lightweight neural network on annotated samples, and run inference on video frames.

## Project Overview

The main goal of the project is to support the search and analysis of unique objects in a video stream through a compact desktop workflow:

- load and inspect a video frame by frame
- create object classes interactively inside the application
- assign a unique color to each class
- annotate frame regions with grid-based masks
- save masks for later reuse and training
- train a simple feed-forward neural network on labeled frame patches
- load previously saved weights and run inference over a video

The project was built as an experimental computer vision and annotation tool rather than a production-ready platform. It is best understood as a practical research prototype demonstrating a full pipeline from manual labeling to lightweight model-based analysis.

## Features

- **Video navigation**
  - Open a local video file and move through frames manually.
  - Play forward, play backward, and pause playback.

- **Interactive annotation**
  - Create custom object categories directly in the UI.
  - Paint and erase labeled regions on a grid aligned with the frame.
  - Store frame masks as image files for later training.

- **Mask persistence**
  - Masks are saved per frame and can be reloaded automatically.
  - The application keeps annotation data tied to the selected video.

- **Neural network workflow**
  - Train a lightweight custom neural network on grayscale frame patches.
  - Save learned weights to `weights.txt`.
  - Load saved weights and run prediction across video frames.

- **Visual analysis**
  - Overlay annotated regions on top of the original frame.
  - Render prediction output as colored blocks for quick inspection.

## Tech Stack

- **Language:** C++
- **Desktop UI:** Win32 API
- **Computer vision:** OpenCV 4.2
- **Modeling:** custom feed-forward neural network implementation
- **Build environment:** Visual Studio 2022 / MSVC toolset (`v143`)

## Repository Structure

- `Searching and Analyzing Unique Objects in a Video Stream.sln`
  - Visual Studio solution
- `Searching and Analyzing Unique Objects in a Video Stream/`
  - application source code
- `packages/`
  - OpenCV package dependencies used by the project
- `x64/`
  - local build artifacts

## How It Works

At a high level, the application follows this workflow:

1. Open a video file.
2. Navigate to frames of interest.
3. Add object types in the UI.
4. Paint object regions on frame masks using color-coded labels.
5. Save annotation masks automatically as frame images.
6. Train the built-in neural network on labeled patches extracted from frames.
7. Load trained weights and run inference to visualize predicted object regions.

Internally, the application stores mask images per frame and converts frame patches into grayscale inputs for the neural network. During inference, the model predicts the most likely class for each grid block and overlays the result on top of the video.

## Build and Run

### Requirements

- Windows
- Visual Studio 2022
- MSVC toolset `v143`
- OpenCV 4.2 package files available in the repository

### Build

1. Open `Searching and Analyzing Unique Objects in a Video Stream.sln` in Visual Studio.
2. Restore NuGet packages if Visual Studio requests it.
3. Build the solution in the desired configuration, typically `Debug | x64`.

### Run

1. Start the application from Visual Studio or from the compiled executable.
2. Select a local video file when prompted.
3. Use the controls to inspect frames, annotate objects, train the model, or run inference.

## Notes and Current Status

- This project was created as a bachelor's thesis prototype and is preserved as a portfolio/research repository.
- The neural-network component is intentionally lightweight and educational rather than optimized for large-scale production use.
- Runtime behavior can depend on the local Windows/OpenCV/video-backend environment, especially when loading external video files.

## Why This Project Matters

This repository demonstrates:

- desktop application development in modern C++
- integration of Win32 UI and OpenCV-based video processing
- interactive annotation workflow design
- custom neural-network implementation without external ML frameworks
- an end-to-end research prototype from labeling to inference

## License

This project is licensed under the MIT License. See the [LICENSE](LICENSE) file for details.
