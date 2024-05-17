/*
 Preservation Virtual Machine Project
 Yet another ivm emulator
*/

/*
 I/O routines from:
 https://github.com/preservationvm/ivm-implementations/tree/master/OtherMachines 
*/

// IO instructions
#include <png.h>
#include <dirent.h> // scandir()

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

/* Based on https://stackoverflow.com/a/22059317. */
static long readFile(char* filename, void* start) {
  FILE* fileptr = fopen(filename, "rb");
  if (!fileptr) {
    fprintf(stderr, "File not found or not readable: %s\n", filename);
    exit(NOT_READABLE);
  }
  fseek(fileptr, 0, SEEK_END);
  long filelen = ftell(fileptr);
  rewind(fileptr);

  size_t actual = fread(start, 1, filelen, fileptr);
  if (actual < filelen) {
    fprintf(stderr, "Partially read: %s\n", filename);
    exit(NOT_READABLE);
  }
  fclose(fileptr);
  return filelen;
}

static void writeFile(char* filename, void* start, size_t size, int append) {
  FILE* fileptr = fopen(filename, append?"ab":"wb");
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

static const WavHeader wavBasicHeader = {
  {'R', 'I', 'F', 'F'}, 36U, {'W', 'A', 'V', 'E'},
  {'f', 'm', 't', ' '}, 16U, 1U, 2U, 0U, 0U, 4U, 16U,
  {'d', 'a', 't', 'a'}, 0U
};

static void writeWav(char* filename, void* start, size_t size, uint32_t sampleRate) {
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

static void writePng(char* filename, void* start, uint16_t width, uint16_t height) {
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

static void spaceInit(Space* s) {
  s->array = NULL;
  s->size = s->used = 0;
}

static void spaceReset(Space* s, size_t needed) {
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

static void bytesInit(Bytes* b, size_t initialSize) {
  b->array = malloc(initialSize);
  if (!b->array) exit(OUT_OF_MEMORY);
  b->size = initialSize;
  b->used = 0;
}

static void bytesMakeSpace(Bytes* b, size_t extra) {
  if (b->used + extra > b->size) {
    b->size += extra > b->size ? extra : b->size;
    b->array = realloc(b->array, b->size);
    if (!b->array) exit(OUT_OF_MEMORY);
  }
}

static void bytesPutByte(Bytes* b, uint8_t x) {
    bytesMakeSpace(b, 1);
    b->array[b->used++] = x;
}

// UTF-32 to UTF-8
static void bytesPutChar(Bytes* b, uint32_t c) {
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

// Read UTF-32 character, assuming UTF-8 input.
// NB. Actual EOF is converted into the EOF character (^D).
static uint32_t ioReadChar() {
  const uint32_t eof = (uint32_t) 4;
  int c0 = getc(stdin);
  if (c0 == EOF) return eof;
  uint32_t u0 = (uint32_t) c0;
  if (c0 < 0x80) return u0;
  u0 &= 0x1f;
  int c1 = getc(stdin);
  if (c1 == EOF) return eof;
  uint32_t u1 = (uint32_t) (c1 & 0x3f);
  if (c0 < 0xe0) return (u0 << 6) + u1;
  u0 &= 0x0f; //*uma
  int c2 = getc(stdin);
  if (c2 == EOF) return eof;
  uint32_t u2 = (uint32_t) (c2 & 0x3f);
  if (c0 < 0xf0) return (u0 << 12) + (u1 << 6) + u2;
  u0 &= 0x07; //*uma
  int c3 = getc(stdin);
  if (c3 == EOF) return eof;
  uint32_t u3 = (uint32_t) (c3 & 0x3f);
  return (u0 << 18) + (u1 << 12) + (u2 << 6) + u3;
}

static void bytesPutSample(Bytes* b, uint16_t left, uint16_t right) {
  bytesMakeSpace(b, 4);
  uint16_t* pos = (uint16_t*) (b->array + b->used);
  pos[0] = left;
  pos[1] = right;
  b->used += 4;
}


/* Input state */

// 16 MiB
#define INITIAL_IN_IMG_SIZE 0x1000000

static struct dirent** inpFiles;
static int numInpFiles;
static Space currentInImage;
static size_t currentInRowbytes = 0;
static Space currentInRowpointers;

static int acceptPng(const struct dirent* entry) {
  char* ext;
  return (entry->d_type == DT_REG)
    && (ext = strrchr(entry->d_name, '.'))
    && strcmp(ext, ".png") == 0;
}

static void ioInitIn() {
  numInpFiles = inpDir ? scandir(inpDir, &inpFiles, acceptPng, alphasort) : 0;
  if (numInpFiles < 0) {
    perror("scandir");
    exit(NOT_READABLE);
  }
  spaceInit(&currentInImage);
  spaceInit(&currentInRowpointers);
}

static void ioReadFrame(uint64_t i, uint64_t* width, uint64_t* height) {
  /*uma: if inpDir is the same as outDir, update numImpFiles because 
         new frames could have been generated*/
  //if (inpDir && outDir && !strcmp(inpDir, outDir)){
      numInpFiles = inpDir ? scandir(inpDir, &inpFiles, acceptPng, alphasort) : 0;
  //}
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
  //*uma: fixed bug
  if (*height * sizeof(void*) > currentInRowpointers.size) {
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

static uint8_t ioReadPixel(uint16_t x, uint16_t y) {
  return *((uint8_t*) currentInImage.array + currentInRowbytes * y + x);
}


/* Output state */

// 16 MiB
#define INITIAL_TEXT_SIZE    0x1000000
#define INITIAL_BYTES_SIZE   0x1000000
#define INITIAL_SAMPLES_SIZE 0x1000000
#define INITIAL_OUT_IMG_SIZE 0x1000000

static int outputCounter = 0;
static Bytes currentText;
static Bytes currentBytes;
static Bytes currentSamples;
static uint32_t currentSampleRate;
static Space currentOutImage;
static uint16_t currentOutWidth;
static uint16_t currentOutHeight;

static void ioInitOut() {
  if (outDir) {
    if (strlen(outDir) + 16 > MAX_FILENAME) {
      exit(STRING_TOO_LONG);
    }
  }
  bytesInit(&currentText, INITIAL_TEXT_SIZE);
  bytesInit(&currentBytes, INITIAL_BYTES_SIZE);
  bytesInit(&currentSamples, INITIAL_SAMPLES_SIZE);
  spaceInit(&currentOutImage);
}

static void ioFlush_console() {  //*uma: flush current cumulative text without increasing frame number
  static int outputCounter_cur = -1;
  if (outDir) {
    static char filename[MAX_FILENAME];
    char* ext = filename + sprintf(filename, "%s/%08d.", outDir, outputCounter);

    int append = outputCounter_cur == outputCounter;

    if (currentText.used > 0) {
      sprintf(ext, "text");
      writeFile(filename, currentText.array, currentText.used, append);
    }
    if (currentBytes.used > 0) {
      sprintf(ext, "bytes");
      writeFile(filename, currentBytes.array, currentBytes.used, append);
    }
  }
  currentText.used = 0;
  currentBytes.used = 0;
  outputCounter_cur = outputCounter;
}

static void ioFlush() {
  if (outDir) {
    static char filename[MAX_FILENAME];
    char* ext = filename + sprintf(filename, "%s/%08d.", outDir, outputCounter);
    //~ if (currentText.used > 0) {
    //~   sprintf(ext, "text");
    //~   //writeFile(filename, currentText.array, currentText.used);
    //~   writeFile_append(filename, currentText.array, currentText.used); //*uma
    //~ }
    //~ if (currentBytes.used > 0) {
    //~   sprintf(ext, "bytes");
    //~   //writeFile(filename, currentBytes.array, currentBytes.used);
    //~   writeFile_append(filename, currentBytes.array, currentBytes.used); //*uma
    //~ }

    ioFlush_console(); //*uma

    if (currentSamples.used > 0) {
      sprintf(ext, "wav");
      writeWav(filename, currentSamples.array, currentSamples.used, currentSampleRate);
    }
    if (currentOutImage.used > 0) {
      sprintf(ext, "png");
      writePng(filename, currentOutImage.array, currentOutWidth, currentOutHeight);
    }
  }
  currentText.used = 0;
  currentBytes.used = 0;
  currentSamples.used = 0;
  currentOutImage.used = 0;
  outputCounter++;
}


static void ioPutChar(uint32_t c) {
  int start = currentText.used;
  bytesPutChar(&currentText, c);
  int len = currentText.used - start;
  //printf("%.*s", len, currentText.array + start); //original
  fprintf(stderr, "%.*s", len, currentText.array + start); //*uma: put_char to stderr as "ivm run" does
  if (currentText.used >= INITIAL_TEXT_SIZE){ //*uma: flush console if buffer exhausted
    ioFlush_console();
  }
}

static void ioPutByte(uint8_t x) {
  bytesPutByte(&currentBytes, x);
  if (currentBytes.used >= INITIAL_BYTES_SIZE){ //*uma: flush console if buffer exhausted
    ioFlush_console();
  }
}

static void ioAddSample(uint16_t left, uint16_t right) {
  bytesPutSample(&currentSamples, left, right);
}

static void ioNewFrame(uint16_t width, uint16_t height, uint32_t sampleRate) {
  currentSampleRate = sampleRate;
  currentOutWidth = width;
  currentOutHeight = height;
  spaceReset(&currentOutImage, 3 * width * height);
}

static void ioSetPixel(uint16_t x, uint16_t y, uint64_t r, uint64_t g, uint64_t b) {
  uint8_t* p = currentOutImage.array + (y * currentOutWidth + x) * 3;
  p[0] = (uint8_t) r;
  p[1] = (uint8_t) g;
  p[2] = (uint8_t) b;
}
