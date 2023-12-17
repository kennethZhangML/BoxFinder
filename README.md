# BoxFinder: BMP Image Edge Detection and Annotation

BoxFinder is a C program designed to detect and annotate rectangular objects in BMP images. It utilizes edge detection to find potential objects and marks them in the image.

## Functions

### `Image* loadBMP(const char* filename)`

Loads a BMP image from the specified file.

- **Parameters**
  - `filename`: Path to the BMP file.
- **Returns**
  - A pointer to an `Image` structure containing the loaded image data.

### `double colorMagnitudeDiff(unsigned char* pixel1, unsigned char* pixel2)`

Calculates the color magnitude difference between two pixels.

- **Parameters**
  - `pixel1`: Pointer to the first pixel.
  - `pixel2`: Pointer to the second pixel.
- **Returns**
  - The Euclidean distance between the two pixels in color space.

### `void detectHorizontalEdges(Image* img, int y, double magThresh, int* leftEdge, int* rightEdge)`

Detects horizontal edges in a given row of the image.

- **Parameters**
  - `img`: Pointer to the image.
  - `y`: The row in the image to check.
  - `magThresh`: Threshold for detecting an edge.
  - `leftEdge`: Pointer to store the left edge position.
  - `rightEdge`: Pointer to store the right edge position.

### `void calculateCentroid(int top, int bottom, int left, int right, double* centroidX, double* centroidY)`

Calculates the centroid of a box defined by its edges.

- **Parameters**
  - `top`, `bottom`, `left`, `right`: Edges of the box.
  - `centroidX`, `centroidY`: Pointers to store the centroid coordinates.

### `void drawRectangle(Image* img, Box box, unsigned char color[3])`

Draws a rectangle on the image.

- **Parameters**
  - `img`: Pointer to the image.
  - `box`: The box to draw.
  - `color`: Color of the rectangle (RGB).

### `void saveBMP(Image* img, const char* filename)`

Saves the image to a BMP file.

- **Parameters**
  - `img`: Pointer to the image.
  - `filename`: Path where the image should be saved.

### `void detectBoxes(Image* img, double magThresh, int minWidth, int maxWidth, Box* box)`

Detects boxes in the image based on edge detection.

- **Parameters**
  - `img`: Pointer to the image.
  - `magThresh`: Threshold for detecting an edge.
  - `minWidth`, `maxWidth`: Minimum and maximum width for a potential box.
  - `box`: Pointer to store the detected box.

## Usage

1. Compile the program using a C compiler.
2. Run the program, providing a BMP image file as input.
3. The program will detect and annotate rectangular objects and save the modified image.

## Contributing

Contributions are welcome. Please feel free to fork the repository and submit pull requests.

## License

This project is open source and available under the [MIT License](LICENSE).

