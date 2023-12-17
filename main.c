#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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

Image* loadBMP(const char* filename);
void applyBilateralFilter(Image* img);
void convertToGrayscale(Image* img);
void applyUnsharpMask(Image* img);
void convertToASCIIArt(const Image* img, char* asciiArt);
void applySobelEdgeDetection(Image* img);

const char ascii_chars[] = " .:-=+*#%@";

int clamp(int val, int min, int max) {
    if (val < min) {
        return min;
    } else if (val > max) {
        return max;
    } else {
        return val;
    }
}

void applySobelEdgeDetection(Image* img) {
    int gx, gy, sum;
    unsigned char* edgeData = (unsigned char*)malloc(img->width * img->height);

    for (int y = 1; y < img->height - 1; y++) {
        for (int x = 1; x < img->width - 1; x++) {
            gx = -1 * img->data[((y - 1) * img->width + (x - 1)) * 3] - 2 * img->data[(y * img->width + (x - 1)) * 3] - 1 * img->data[((y + 1) * img->width + (x - 1)) * 3]
                + 1 * img->data[((y - 1) * img->width + (x + 1)) * 3] + 2 * img->data[(y * img->width + (x + 1)) * 3] + 1 * img->data[((y + 1) * img->width + (x + 1)) * 3];
            gy = -1 * img->data[((y - 1) * img->width + (x - 1)) * 3] - 2 * img->data[((y - 1) * img->width + x) * 3] - 1 * img->data[((y - 1) * img->width + (x + 1)) * 3]
                + 1 * img->data[((y + 1) * img->width + (x - 1)) * 3] + 2 * img->data[((y + 1) * img->width + x) * 3] + 1 * img->data[((y + 1) * img->width + (x + 1)) * 3];
            sum = abs(gx) + abs(gy);
            edgeData[y * img->width + x] = (sum > 255) ? 255 : sum;
        }
    }

    for (int i = 0; i < img->width * img->height; i++) {
        img->data[i * 3] = img->data[i * 3 + 1] = img->data[i * 3 + 2] = edgeData[i];
    }
    free(edgeData);
}

void applyBilateralFilter(Image* img) {
    const int radius = 5;  
    const double sigmaI = 12.0;  
    const double sigmaS = 16.0;  

    unsigned char* filteredData = (unsigned char*)malloc(img->width * img->height * 3);

    for (int i = 0; i < img->height; i++) {
        for (int j = 0; j < img->width; j++) {
            double weightSum = 0.0;
            double rSum = 0.0, gSum = 0.0, bSum = 0.0;

            for (int dy = -radius; dy <= radius; dy++) {
                for (int dx = -radius; dx <= radius; dx++) {
                    int y = clamp(i + dy, 0, img->height - 1);
                    int x = clamp(j + dx, 0, img->width - 1);

                    double dr = img->data[(y * img->width + x) * 3 + 0] - img->data[(i * img->width + j) * 3 + 0];
                    double dg = img->data[(y * img->width + x) * 3 + 1] - img->data[(i * img->width + j) * 3 + 1];
                    double db = img->data[(y * img->width + x) * 3 + 2] - img->data[(i * img->width + j) * 3 + 2];

                    double spatialWeight = exp(-(dx * dx + dy * dy) / (2 * sigmaS * sigmaS));
                    double rangeWeight = exp(-(dr * dr + dg * dg + db * db) / (2 * sigmaI * sigmaI));
                    double weight = spatialWeight * rangeWeight;

                    rSum += img->data[(y * img->width + x) * 3 + 0] * weight;
                    gSum += img->data[(y * img->width + x) * 3 + 1] * weight;
                    bSum += img->data[(y * img->width + x) * 3 + 2] * weight;
                    weightSum += weight;
                }
            }

            filteredData[(i * img->width + j) * 3 + 0] = (unsigned char)(rSum / weightSum);
            filteredData[(i * img->width + j) * 3 + 1] = (unsigned char)(gSum / weightSum);
            filteredData[(i * img->width + j) * 3 + 2] = (unsigned char)(bSum / weightSum);
        }
    }

    memcpy(img->data, filteredData, img->width * img->height * 3);
    free(filteredData);
}


void convertToGrayscale(Image* img) {
    for (int i = 0; i < img->width * img->height; i++) {
        int gray = (img->data[i * 3] + img->data[i * 3 + 1] + img->data[i * 3 + 2]) / 3;
        img->data[i * 3] = img->data[i * 3 + 1] = img->data[i * 3 + 2] = gray;
    }
}

void applyUnsharpMask(Image* img) {
    Image* original = malloc(sizeof(Image));
    original->width = img->width;
    original->height = img->height;
    original->data = malloc(img->width * img->height * 3);
    memcpy(original->data, img->data, img->width * img->height * 3);

    applyBilateralFilter(original);

    for (int i = 0; i < img->width * img->height; i++) {
        int r = img->data[i * 3] - original->data[i * 3];
        int g = img->data[i * 3 + 1] - original->data[i * 3 + 1];
        int b = img->data[i * 3 + 2] - original->data[i * 3 + 2];
        img->data[i * 3] = clamp(img->data[i * 3] + r, 0, 255);
        img->data[i * 3 + 1] = clamp(img->data[i * 3 + 1] + g, 0, 255);
        img->data[i * 3 + 2] = clamp(img->data[i * 3 + 2] + b, 0, 255);
    }

    free(original->data);
    free(original);
}

void convertToASCIIArt(const Image* img, char* asciiArt) {
    for (int i = 0; i < img->height; i++) {
        for (int j = 0; j < img->width; j++) {
            int gray = img->data[(i * img->width + j) * 3];
            char character = (gray > 128) ? ' ' : '#'; // Use simple binary threshold for clarity
            asciiArt[i * img->width + j] = character;
        }
        asciiArt[i * img->width + img->width] = '\n';
    }
    asciiArt[img->height * img->width] = '\0';
}


int main() {
    Image* img = loadBMP("turtle.bmp");
    if (img == NULL) {
        fprintf(stderr, "Error loading image.\n");
        return 1;
    }

    applySobelEdgeDetection(img);

    char* asciiArt = (char*)malloc(img->width * img->height + img->height + 1);
    convertToASCIIArt(img, asciiArt);
    printf("%s", asciiArt);

    free(asciiArt);
    free(img->data);
    free(img);
    return 0;
}
