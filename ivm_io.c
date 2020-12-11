/*
 Preservation Virtual Machine Project

 Original Author:
    Ivar Rummelhoff
    from github.com/preservationvm/ivm-implementations/blob/master/OtherMachines/vm.c

 Reentrant version:
    Sergio Romero Montiel
    Eladio Gutierrez Carrasco
    Oscar Plata Gonzalez

 Date: May 2020

 IVM input/output instruction implementation file
*/

#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <png.h>
#include <dirent.h> // scandir()
#include <string.h> //bzero?
#include "ivm_io.h"

#define OUTPUT_PUTCHAR stderr
#define MAX_FILENAME 260

// Error codes
#define OPTION_PARSE_ERROR 1
#define NOT_READABLE 2
#define NOT_IMPLEMENTED 3
#define UNDEFINED_INSTRUCTION 4
#define OUT_OF_MEMORY 5
#define STRING_TOO_LONG 6
#define NOT_WRITEABLE 7
#define PNG_TROUBLE 8


void writeFile(char* filename, void* start, size_t size) {
  FILE* fileptr = fopen(filename, "wb");
  if (!fileptr || fwrite(start, 1, size, fileptr) < size) {
    fprintf(stderr, "Trouble writing: %s\n", filename);
    exit(NOT_WRITEABLE);
  }
  fclose(fileptr);
}

// http://www-mmsp.ece.mcgill.ca/Documents/AudioFormats/WAVE/WAVE.html
typedef struct
{
    uint8_t  chunkId1[4];
    uint32_t chunkSize1;
    uint8_t  format[4];
    uint8_t  chunkId2[4];
    uint32_t chunkSize2;
    uint16_t audioFormat;
    uint16_t numChannels;
    uint32_t sampleRate;
    uint32_t byteRate;
    uint16_t blockAlign;
    uint16_t bitsPerSample;
    uint8_t  chunkId3[4];
    int32_t  chunkSize3;
} WavHeader;

const WavHeader wavBasicHeader = {
  {'R', 'I', 'F', 'F'}, 36U, {'W', 'A', 'V', 'E'},
  {'f', 'm', 't', ' '}, 16U, 1U, 2U, 0U, 0U, 4U, 16U,
  {'d', 'a', 't', 'a'}, 0U
};

void writeWav(char* filename, void* start, size_t size, uint32_t sampleRate) {
  FILE* fileptr = fopen(filename, "wb");
  if (!fileptr) {
    fprintf(stderr, "Trouble writing: %s\n", filename);
    exit(NOT_WRITEABLE);
  }
  WavHeader h = wavBasicHeader;
  h.chunkSize1 += size;
  h.sampleRate = sampleRate;
  h.byteRate = 4U * sampleRate;
  h.chunkSize3 = size;
  fwrite((const void*) &h, sizeof(WavHeader), 1, fileptr);
  if (fwrite(start, 1, size, fileptr) < size) {
    fprintf(stderr, "Trouble writing: %s\n", filename);
    exit(NOT_WRITEABLE);
  }
  fclose(fileptr);
}

void writePng(char* filename, void* start, uint16_t width, uint16_t height) {
  png_structp png;
  png_infop info;
  if (!(png = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL))
  || !(info = png_create_info_struct(png))
  || setjmp(png_jmpbuf(png))) {
    exit(PNG_TROUBLE);
  }
  FILE *fileptr = fopen(filename, "wb");
  if (!fileptr) {
    fprintf(stderr, "Trouble writing: %s\n", filename);
    exit(NOT_WRITEABLE);
  }
  png_init_io(png, fileptr);
  png_set_IHDR(
    png,
    info,
    width, height,
    8, // 8-bit color depth
    PNG_COLOR_TYPE_RGB,
    PNG_INTERLACE_NONE,
    PNG_COMPRESSION_TYPE_DEFAULT,
    PNG_FILTER_TYPE_DEFAULT
  );
  png_write_info(png, info);
  for (int y = 0; y < height; y++) {
    png_write_row(png, start + y * width * 3);
  }
  png_write_end(png, NULL);
  fclose(fileptr);
  png_destroy_write_struct(&png, &info);
}


/* Reusable memory */

typedef struct {
  void* array;
  size_t size;
  size_t used;
} Space;

void spaceInit(Space* s) {
  s->array = NULL;
  s->size = s->used = 0;
}

void spaceReset(Space* s, size_t needed) {
  s->used = needed;
  if (s->size >= needed) return;
  free(s->array);
  s->array = malloc(needed);
  if (!s->array) exit(OUT_OF_MEMORY);
  s->size = needed;
}


/* Growing byte buffer */

typedef struct {
  uint8_t* array;
  size_t size;
  size_t used;
} Bytes;

void bytesInit(Bytes* b, size_t initialSize) {
  b->array = malloc(initialSize);
  if (!b->array) exit(OUT_OF_MEMORY);
  b->size = initialSize;
  b->used = 0;
}

void bytesMakeSpace(Bytes* b, size_t extra) {
  if (b->used + extra > b->size) {
    b->size += extra > b->size ? extra : b->size;
    b->array = realloc(b->array, b->size);
    if (!b->array) exit(OUT_OF_MEMORY);
  }
}

void bytesPutByte(Bytes* b, uint8_t x) {
    bytesMakeSpace(b, 1);
    b->array[b->used++] = x;
}

// UTF-32 to UTF-8
void bytesPutChar(Bytes* b, uint32_t c) {
  if (c < 0x80) {
    bytesMakeSpace(b, 1);
    b->array[b->used++] = (uint8_t) c;
  } else if (c < 0x800) {
    bytesMakeSpace(b, 2);
    b->array[b->used++] = (uint8_t) (0xc0 | c >> 6);
    b->array[b->used++] = (uint8_t) (0x80 | (0x3f & c));
  } else if (c < 0x10000) {
    bytesMakeSpace(b, 3);
    b->array[b->used++] = (uint8_t) (0xe0 | c >> 12);
    b->array[b->used++] = (uint8_t) (0x80 | (0x3f & c >> 6));
    b->array[b->used++] = (uint8_t) (0x80 | (0x3f & c));
  } else {
    bytesMakeSpace(b, 4);
    b->array[b->used++] = (uint8_t) (0xf0 | (0x07 & c >> 18)); // &: in case is > 21 bits
    b->array[b->used++] = (uint8_t) (0x80 | (0x3f & c >> 12));
    b->array[b->used++] = (uint8_t) (0x80 | (0x3f & c >> 6));
    b->array[b->used++] = (uint8_t) (0x80 | (0x3f & c));
  }
}

void bytesPutSample(Bytes* b, uint16_t left, uint16_t right) {
  bytesMakeSpace(b, 4);
  uint16_t* pos = (uint16_t*) (b->array + b->used);
  pos[0] = left;
  pos[1] = right;
  b->used += 4;
}


/* Input state */

// 16 MiB
#define INITIAL_IN_IMG_SIZE 0x1000000

extern char* inpDir;
extern char* outDir;

struct dirent** inpFiles;
int numInpFiles;
Space currentInImage;
size_t currentInRowbytes = 0;
Space currentInRowpointers;

int acceptPng(const struct dirent* entry) {
  char* ext;
  return (entry->d_type == DT_REG)
    && (ext = strrchr(entry->d_name, '.'))
    && strcmp(ext, ".png") == 0;
}

void ioInitIn() {
  numInpFiles = inpDir ? scandir(inpDir, &inpFiles, acceptPng, alphasort) : 0;
  if (numInpFiles < 0) {
    perror("scandir");
    exit(NOT_READABLE);
  }
  spaceInit(&currentInImage);
  spaceInit(&currentInRowpointers);
}

void ioReadFrame(uint64_t i, uint64_t* width, uint64_t* height) {
  /*uma: update numImpFiles because new frames could have been generated*/
  numInpFiles = inpDir ? scandir(inpDir, &inpFiles, acceptPng, alphasort) : 0;
  if (i >= numInpFiles) {
    *width = 0;
    *height = 0;
    return;
  }
  static char filename[MAX_FILENAME];
  struct dirent* f = inpFiles[i];
  sprintf(filename, "%s/%s", inpDir, f->d_name);

  FILE *fileptr;
  png_structp png;
  png_infop info;
  if (!(fileptr = fopen(filename, "rb"))
  || !(png = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL))
  || !(info = png_create_info_struct(png))
  || setjmp(png_jmpbuf(png))) {
    perror("png");
    exit(NOT_READABLE);
  }
  png_init_io(png, fileptr);
  png_read_info(png, info);
  *width = png_get_image_width(png, info);
  *height = png_get_image_height(png, info);

  if (png_get_bit_depth(png, info) == 16) {
    png_set_strip_16(png);
  }
  png_byte color_type = png_get_color_type(png, info);
  if (color_type == PNG_COLOR_TYPE_RGB ||
      color_type == PNG_COLOR_TYPE_RGB_ALPHA) {
    png_set_rgb_to_gray(png, 1, -1.0, -1.0); // Default weights
  }
  if (color_type & PNG_COLOR_MASK_ALPHA) {
    png_set_strip_alpha(png);
  }
  png_read_update_info(png, info);
  size_t rowbytes = png_get_rowbytes(png, info);
  size_t needed = rowbytes * *height;

  // We have attempted to optimize for the expected case
  // when all the inp frames have the same format.

  if (needed > currentInImage.size) {
    spaceReset(&currentInImage, needed);
    currentInRowbytes = 0;
  }
  if (*height*sizeof(void*) > currentInRowpointers.size) {
    spaceReset(&currentInRowpointers, sizeof(void*) * *height);
    currentInRowbytes = 0;
  }
  if (rowbytes != currentInRowbytes) {
    void** rp = currentInRowpointers.array;
    for (int y = 0; y < *height; y++) {
      rp[y] = currentInImage.array + rowbytes * y;
    }
    currentInRowbytes = rowbytes;
  }

  png_read_image(png, currentInRowpointers.array);
  fclose(fileptr);
  png_destroy_read_struct(&png, &info, NULL);
}

uint8_t ioReadPixel(uint16_t x, uint16_t y) {
  return *((uint8_t*) currentInImage.array + currentInRowbytes * y + x);
}


/* Output state */

// 16 MiB
#define INITIAL_TEXT_SIZE 0x1000000
#define INITIAL_BYTES_SIZE 0x1000000
#define INITIAL_SAMPLES_SIZE 0x1000000
#define INITIAL_OUT_IMG_SIZE 0x1000000

volatile int outputCounter = 0;
struct OutputElement{
    int number;
    Bytes text;
    Bytes bytes;
    Bytes samples;
    uint32_t sampleRate;
    Space outImage;
    uint16_t outWidth;
    uint16_t outHeight;
};

// the public output element
OutputElement_t current = NULL;

// reentrant flush
void ioFlush_r(OutputElement_t ioElem)
{
    if (outDir) {
        char filename[MAX_FILENAME];
        char* ext = filename + sprintf(filename, "%s/%08d.", outDir, ioElem->number);
        if (ioElem->text.used > 0) {
            sprintf(ext, "text");
            writeFile(filename, ioElem->text.array, ioElem->text.used);
        }
        if (ioElem->bytes.used > 0) {
            sprintf(ext, "bytes");
            writeFile(filename, ioElem->bytes.array, ioElem->bytes.used);
        }
        if (ioElem->samples.used > 0) {
            sprintf(ext, "wav");
            writeWav(filename, ioElem->samples.array, ioElem->samples.used, ioElem->sampleRate);
        }
        if (ioElem->outImage.used > 0) {
            sprintf(ext, "png");
            writePng(filename, ioElem->outImage.array, ioElem->outWidth, ioElem->outHeight);
            // clear png, unset pixels are set to 0
            //~ bzero(&ioElem->outImage,ioElem->outImage.used);
        }
    }
    ioElem->number = 0;
    ioElem->text.used = 0;
    ioElem->bytes.used = 0;
    ioElem->samples.used = 0;
    ioElem->outImage.used = 0;
}

// reentrant ioInitElem
void ioInitElem(OutputElement_t ioElem)
{
    ioElem->number = 0;
    ioElem->text.used = 0;
    ioElem->bytes.used = 0;
    ioElem->samples.used = 0;
    ioElem->outImage.used = 0;
    bytesInit(&ioElem->text, INITIAL_TEXT_SIZE);
    bytesInit(&ioElem->bytes, INITIAL_BYTES_SIZE);
    bytesInit(&ioElem->samples, INITIAL_SAMPLES_SIZE);
    spaceInit(&ioElem->outImage);
}

OutputElement_t newOutputElement()
{
    OutputElement_t ioElem = malloc(sizeof(struct OutputElement));
    ioInitElem(ioElem);
    return ioElem;
}

// instrumented for parallel output
void ioInitOut() {
    if (outDir) {
        if (strlen(outDir) + 16 > MAX_FILENAME) {
            exit(STRING_TOO_LONG);
        }
    }
    current = newOutputElement();
    #ifdef PARALLEL_OUTPUT
    ioInitParallel((VoidFunct_t)newOutputElement,(Funct_t)ioFlush_r);
    #endif
}

void ioNewFrame(uint16_t width, uint16_t height, uint32_t sampleRate) {
    current->number = ++outputCounter;
    current->sampleRate = sampleRate;
    current->outWidth = width;
    current->outHeight = height;
    spaceReset(&current->outImage, 3 * width * height);
}

// TODO: Remove the printf?
void ioPutChar(uint32_t c) {
    int start = current->text.used;
    bytesPutChar(&current->text, c);
    int len = current->text.used - start;
    fprintf(OUTPUT_PUTCHAR, "%.*s", len, current->text.array + start);
}

void ioPutByte(uint8_t x) {
    bytesPutByte(&current->bytes, x);
}

void ioAddSample(uint16_t left, uint16_t right) {
    bytesPutSample(&current->samples, left, right);
}

void ioSetPixel(uint16_t x, uint16_t y, uint64_t r, uint64_t g, uint64_t b) {
  uint8_t* p = current->outImage.array + (y * current->outWidth + x) * 3;
  p[0] = (uint8_t) r;
  p[1] = (uint8_t) g;
  p[2] = (uint8_t) b;
}

