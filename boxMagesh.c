#include <stdio.h>
#include <stdlib.h>
#include <math.h>

typedef struct {
    int width;
    int height;
    unsigned char* data;  
} Image;

#pragma pack(push, 1)
typedef struct {
    unsigned short type;
    unsigned int size;
    unsigned short reserved1, reserved2;
    unsigned int offset;
} BMPHeader;

typedef struct {
    int top, bottom, left, right;
} Box;

typedef struct {
    unsigned int size;
    int width, height;
    unsigned short planes;
    unsigned short bitCount;
    unsigned int compression;
    unsigned int sizeImage;
    int xPelsPerMeter;
    int yPelsPerMeter;
    unsigned int clrUsed;
    unsigned int clrImportant;
} BMPImageHeader;
#pragma pack(pop)

Image* loadBMP(const char* filename) {
    FILE* file = fopen(filename, "rb");
    if (file == NULL) {
        fprintf(stderr, "Unable to open file \"%s\"\n", filename);
        return NULL;
    }

    BMPHeader bmpHeader;
    BMPImageHeader bmpImageHeader;

    if (fread(&bmpHeader, sizeof(BMPHeader), 1, file) != 1) {
        fprintf(stderr, "Error reading BMP header.\n");
        fclose(file);
        return NULL;
    }
    if (fread(&bmpImageHeader, sizeof(BMPImageHeader), 1, file) != 1) {
        fprintf(stderr, "Error reading BMP image header.\n");
        fclose(file);
        return NULL;
    }

    printf("BMP Type: 0x%x\n", bmpHeader.type);
    printf("Compression: %u\n", bmpImageHeader.compression);

    if (bmpHeader.type != 0x4D42 || bmpImageHeader.compression != 0) {
        fprintf(stderr, "Unsupported file format or compression.\n");
        fclose(file);
        return NULL;
    }

    Image* img = (Image*)malloc(sizeof(Image));
    img->width = bmpImageHeader.width;
    img->height = bmpImageHeader.height;
    int rowPadded = (img->width * 3 + 3) & (~3);
    img->data = (unsigned char*)malloc(rowPadded * img->height);

    fseek(file, bmpHeader.offset, SEEK_SET);

    for (int i = 0; i < img->height; i++) {
        fread(img->data + (rowPadded * i), 3, img->width, file);
        fseek(file, rowPadded - img->width * 3, SEEK_CUR);
    }

    for (int i = 0; i < img->width * img->height; i++) {
        unsigned char temp = img->data[i * 3];
        img->data[i * 3] = img->data[i * 3 + 2];
        img->data[i * 3 + 2] = temp;
    }

    fclose(file);
    return img;
}

double colorMagnitudeDiff(unsigned char* pixel1, unsigned char* pixel2) {
    return sqrt(pow(pixel1[0] - pixel2[0], 2) + pow(pixel1[1] - pixel2[1], 2) + pow(pixel1[2] - pixel2[2], 2));
}

void detectHorizontalEdges(Image* img, int y, double magThresh, int* leftEdge, int* rightEdge) {
    int runLength = 0;
    *leftEdge = -1;  
    *rightEdge = -1;

    for (int x = 0; x < img->width - 1; x++) {
        if (colorMagnitudeDiff(&img->data[(y * img->width + x) * 3], &img->data[(y * img->width + (x + 1)) * 3]) < magThresh) {
            if (*leftEdge == -1) {
                *leftEdge = x;
            }
            runLength++;
        } else {
            if (runLength > 0 && *leftEdge != -1) {
                *rightEdge = x;
                return; 
            }
            runLength = 0;
            *leftEdge = -1;
        }
    }
}

void calculateCentroid(int top, int bottom, int left, int right, double* centroidX, double* centroidY) {
    *centroidX = (left + right) / 2.0;
    *centroidY = (top + bottom) / 2.0;
}

void drawRectangle(Image* img, Box box, unsigned char color[3]) {
    for (int y = box.top; y <= box.bottom; y++) {
        for (int x = box.left; x <= box.right; x++) {
            if (y == box.top || y == box.bottom || x == box.left || x == box.right) {
                img->data[(y * img->width + x) * 3 + 0] = color[0]; // Red
                img->data[(y * img->width + x) * 3 + 1] = color[1]; // Green
                img->data[(y * img->width + x) * 3 + 2] = color[2]; // Blue
            }
        }
    }
}

void saveBMP(Image* img, const char* filename) {
    FILE* file = fopen(filename, "wb");
    if (file == NULL) {
        fprintf(stderr, "Unable to open file \"%s\" for writing\n", filename);
        return;
    }

    BMPHeader bmpHeader;
    BMPImageHeader bmpImageHeader;

    int rowPadded = (img->width * 3 + 3) & (~3);
    int sizeImage = img->height * rowPadded;

    // Initialize BMPHeader
    bmpHeader.type = 0x4D42; // 'BM'
    bmpHeader.size = sizeof(BMPHeader) + sizeof(BMPImageHeader) + sizeImage;
    bmpHeader.reserved1 = 0;
    bmpHeader.reserved2 = 0;
    bmpHeader.offset = sizeof(BMPHeader) + sizeof(BMPImageHeader);

    // Initialize BMPImageHeader
    bmpImageHeader.size = sizeof(BMPImageHeader);
    bmpImageHeader.width = img->width;
    bmpImageHeader.height = img->height;
    bmpImageHeader.planes = 1;
    bmpImageHeader.bitCount = 24; // 24-bit BMP
    bmpImageHeader.compression = 0;
    bmpImageHeader.sizeImage = sizeImage;
    bmpImageHeader.xPelsPerMeter = 0;
    bmpImageHeader.yPelsPerMeter = 0;
    bmpImageHeader.clrUsed = 0;
    bmpImageHeader.clrImportant = 0;

    fwrite(&bmpHeader, sizeof(BMPHeader), 1, file);
    fwrite(&bmpImageHeader, sizeof(BMPImageHeader), 1, file);

    for (int i = 0; i < img->height; i++) {
        fwrite(img->data + (i * img->width * 3), 3, img->width, file);
        for (int j = 0; j < rowPadded - img->width * 3; j++) {
            fputc(0, file);
        }
    }

    fclose(file);
}


void detectBoxes(Image* img, double magThresh, int minWidth, int maxWidth, Box* box) {
    box->top = -1;
    box->bottom = -1;
    box->left = -1;
    box->right = -1;

    for (int y = 0; y < img->height - 1; y++) {
        int runLength = 0;
        for (int x = 0; x < img->width; x++) {
            if (colorMagnitudeDiff(&img->data[(y * img->width + x) * 3], &img->data[((y + 1) * img->width + x) * 3]) < magThresh) {
                runLength++;
            } else {
                if (runLength >= minWidth && runLength <= maxWidth) {
                    printf("Possible box line detected at Y: %d, Length: %d\n", y, runLength);
                }
                runLength = 0;
            }
        }
    }

    for (int y = 0; y < img->height - 1; y++) {
        int leftEdge, rightEdge;
        detectHorizontalEdges(img, y, magThresh, &leftEdge, &rightEdge);
        if (leftEdge != -1 && rightEdge != -1) {
            int width = rightEdge - leftEdge;
            if (width >= minWidth && width <= maxWidth) {
                box->top = y;
                box->bottom = y + 1;
                box->left = leftEdge;
                box->right = rightEdge;
                return; 
            }
        }
    }
}

int main() {
    Image* img = loadBMP("test_image_objects.bmp");
    if (img == NULL) {
        fprintf(stderr, "Error loading image.\n");
        return 1;
    }

    double magThresh = 30.0; 
    int minWidth = 10;
    int maxWidth = 100;

    Box box;
    detectBoxes(img, magThresh, minWidth, maxWidth, &box);

    if (box.top != -1) {
        unsigned char color[3] = {255, 0, 0}; 
        drawRectangle(img, box, color);
    }

    saveBMP(img, "test_image_objects_mod.bmp");

    free(img->data);
    free(img);

    return 0;
}

