# Orange Vision Analyzer

[Português](README.md) | **English**

**Orange detection, counting and classification in video using C, C++ and OpenCV.**

An academic project developed for the **Computer Vision** course in the second year of the Bachelor's degree in Computer Systems Engineering at the School of Technology of the **Polytechnic Institute of Cávado and Ave (IPCA)**, during the **2025/2026** academic year.

The application analyses each frame of an orange video and overlays the location and characteristics of each fruit, the cumulative count, and an estimated size code and quality category. It uses classical image processing techniques without machine learning models.

## Assignment context

The assignment calls for processing `video.avi` at **1280 × 720** resolution and **25 fps**, using the reference scale of **280 pixels = 55 mm**. Its objectives include counting oranges and determining each fruit's area, perimeter, centre of gravity, bounding box, size code and quality category.

The assignment emphasises implementing image processing operations directly and restricts additional OpenCV functions. In this project, the C library implements colour conversion, segmentation, morphology, connected component labelling and object analysis. OpenCV handles video input, windows and text rendering.

## Features

| Feature | Implementation |
| --- | --- |
| Orange detection | HSV segmentation and filtering by area, circularity and bounding box aspect ratio |
| Mask cleanup | Morphological opening and closing with a 3 × 3 kernel |
| Localisation | Bounding box and cross at the centre of gravity |
| Measurements | Area, perimeter and estimated diameter converted into physical units |
| Counting | Current frame count and cumulative count using temporal association of centres |
| Size grading | Lookup table with 14 size codes, from 0 to 13 |
| Quality category | Estimation based on circularity, shape defect and colour defect percentage |
| Size uniformity | Comparison of the minimum and maximum diameters observed for fruit fully inside the image |
| Visualisation | Annotated video and binary mask in two windows |

## How it works

1. Reads a BGR frame and converts it to HSV using the `vc` library.
2. Creates two masks: one to detect fruit and a stricter one to identify acceptable colour.
3. Applies opening and closing to the detection mask and labels connected components (*blobs*).
4. Filters objects and calculates their area, perimeter, centre, dimensions and shape descriptors.
5. Estimates diameter as the average of the bounding box width and height, then converts measurements using the reference scale.
6. Assigns a size code and quality category, associates centres with previous detections and updates the counts.
7. Draws measurements and statistics over the video.

Temporal association accepts the first available previous centre at a distance of less than **70 pixels**. Missing detections remain in memory for up to **15 frames**, reducing repeated counting during brief detection failures.

### Main parameters

These values are defined near the beginning of `Laranjas.cpp`. H limits are expressed in degrees, while S and V limits are percentages; the library converts them to its internal 8-bit representation.

| Parameter | Value |
| --- | --- |
| Detection mask | H: 10–35°; S: 45–100%; V: 18–100% |
| Acceptable colour mask | H: 13–32°; S: 53–100%; V: 20–100% |
| Minimum accepted area | Greater than 2000 px² |
| Minimum circularity | 0.50 |
| Minimum ratio of the shorter to the longer side | 70% |
| Margin for detecting fruit clipped by the image boundary | 15 px |
| Reference scale | 280 px / 55 mm |

### Implemented classification criteria

All conditions in a row must be met. Rows are evaluated in the order shown:

| Category | Minimum circularity | Maximum shape defect | Maximum colour defect |
| --- | --- | --- | --- |
| Extra | 0.85 | 5% | 4% |
| I | 0.75 | 12% | 10% |
| II | 0.60 | 25% | 25% |

Fruit near the image boundary is labelled `Nao classificada` (not classified). A diameter without a matching table entry receives `Fora calibre` (outside the size range). Remaining fruit that does not meet the criteria receives `Rejeitada` (rejected). The application displays these labels in Portuguese.

Shape defect is calculated from the ratio of the shorter to the longer bounding box side. Colour defect is the percentage of object pixels outside the acceptable colour mask. Circularity is calculated as `4π × area / perimeter²`.

Size ranges overlap; `get_caliber()` returns the first matching interval while iterating through the table from 0 to 13. The uniformity check uses tolerances of 11, 9 or 7 mm, depending on the size code assigned to the largest observed diameter.

## Repository structure

| File | Responsibility |
| --- | --- |
| [Laranjas.cpp](Laranjas.cpp) | Application, frame processing, tracking and results display |
| [vc.c](vc.c) | Image processing and analysis library written in C |
| [vc.h](vc.h) | `IVC` and `OVC` structures and the library's public interface |
| [CMakeLists.txt](CMakeLists.txt) | Build configuration for C and C++ with OpenCV |

The three source files are preserved exactly as supplied in the original ZIP archive. Documentation and build configuration were added to make the project easier to explore and run.

## Build and run

### Requirements

- C11 and C++11 compilers or later.
- OpenCV 4 development headers and libraries, including `core`, `highgui`, `imgproc` and `videoio`.
- CMake 3.16 or later and a compatible build tool.
- A graphical environment to display the windows.
- The original `video.avi`, distributed separately as part of the course.

**The video is not included in this repository or in the supplied source ZIP.** The assignment statement and the citrus quality standards appendix are not distributed in this repository either.

### Using CMake

From the repository root:

```bash
cmake -S . -B build
cmake --build build --config Release
```

If CMake cannot find OpenCV, specify the directory containing `OpenCVConfig.cmake`:

```bash
cmake -S . -B build -DOpenCV_DIR="/path/to/opencv/configuration"
```

Place `video.avi` in the repository root and run the application from that directory:

```bash
# Linux / macOS, using a single-configuration generator
./build/orange_vision
```

```powershell
# Windows, using the Visual Studio generator
.\build\Release\orange_vision.exe
```

The video path is relative to the **current working directory**, rather than the executable's location. In the original version, `video.avi` is hardcoded and there is no command-line argument to select another file.

The application opens the `DETECAO DE LARANJAS` and `MASCARA BINARIA` windows. Press **q** while either window is focused to exit, or wait until the video ends.

### Direct compilation on Linux

With GCC, G++ and the OpenCV development package available through `pkg-config`:

```bash
mkdir -p build
gcc -std=c11 -O2 -c vc.c -o build/vc.o
g++ -std=c++11 -O2 Laranjas.cpp build/vc.o -o build/orange_vision $(pkg-config --cflags --libs opencv4)
./build/orange_vision
```

Compile `vc.c` as C; the C++ application accesses the library using `extern "C"`.

## Limitations and validation

- Colour thresholds, filters and the reference scale are tuned to the assignment video. Changes to lighting, distance, background or resolution require recalibration.
- Size codes and quality categories are estimates derived from a 2D image. The numerical criteria above are project heuristics and do not, by themselves, establish compliance with commercial citrus standards.
- Overlapping fruit may form a single component. Proximity tracking may lose associations or count fruit again during fast motion or prolonged occlusions.
- The uniformity check accumulates extreme measurements across frames; it does not calculate an average per fruit or maintain permanent identifiers.
- Results are displayed as frames are read, using `waitKey(1)`. The displayed FPS value comes from video metadata and is not a measurement of application performance.
- During preparation for publication, `vc.c` passed a syntax check using `gcc -std=c11 -Wall -Wextra -Wpedantic -fsyntax-only`. A full build and execution with the video were not verified because the preparation environment lacked OpenCV and the original video. No measured accuracy or performance results are reported.

## Authors

- **Ana Rita Ranito** — a31511
- **Filipe Fernandes** — a31498

**Lecturer:** Duarte Duque · **Course:** Computer Vision · **Institution:** IPCA / School of Technology.

This repository documents and preserves an academic project. Any use of the source code should respect the institution's authorship and academic integrity rules. No new licence has been assigned to the materials in this repository.
