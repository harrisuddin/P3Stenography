#include<stdio.h>
#include<stdlib.h>
#include<string.h>

int commentsLength = 100; // global to store the size of the comments pointer

struct PPMPixel {
    unsigned int red, green, blue;
};

struct PPM {
    int width, height, max;
    struct PPMPixel *pixels;
    char *comments;
};

/*
Return 1 if a is in arr.
Return 0 if a is not in arr
*/
int containsElement(int a, int arr[], int len) {

    if (len <= 0) {
        return 0;
    }

    for (int i = 0; i < len; i++) {
        if (arr[i] == a) return 1;
    }

    return 0;
}

// return true when i is even
int isEven(int i) { return (i % 2 == 0); }

// return true when i is odd
int isOdd(int i) { return (i % 2 == 1); }

// return true when i and j are both even or odd
int isSame(int i, int j) {
    return ((isEven(i) && isEven(j)) || (isOdd(i) && isOdd(j))); 
}

/* 
append c to the comments pointer in im 
return the length of the pointer
*/
int append(char c, struct PPM *im) {
    int len = strlen(im->comments);
    
    // dynamically allocate more memory to comments
    if (len > commentsLength) {
        commentsLength = 2*commentsLength;
        char *temp = realloc(im->comments, commentsLength*sizeof(char));
        if (!temp) {
            printf("Unable to allocate memory to comments array");
            exit(0);
        } else {
            im->comments = temp;
        }
    }

    // append the char to the comments pointer
    im->comments[len] = c;
    im->comments[len+1] = '\0';
    return len;
}

struct PPM * getPPM(FILE *f) {
    char ch; // Character pointer to grab each character
    struct PPM *im;

    // Check the file is P3
    fscanf(f, "P%c\n", &ch);
    if (ch != '3') {
        fprintf(stderr, "Invalid ppm type.\n");
        exit(0);
    }

    // allocate memory to im
    im = (struct PPM *)malloc(sizeof(struct PPM));
    if (!im) {
        fprintf(stderr, "Unable to allocate memory.\n");
        exit(0);
    }
    
    // allocate memory to comments pointer
    im->comments = malloc(commentsLength*sizeof(char));
    if (!im->comments) {
        printf("Unable to allocate memory to comments pointer.\n");
        exit(0);
    }

    // iterate through the comments and store them
    ch = getc(f);
    while (ch == '#') {
        append(ch, im);
        do {
            ch = getc(f);
            append(ch, im);
        }
        while (ch != '\n');
        ch = getc(f);
    }

    // Return the digit so that it may be extracted wholly
    ungetc(ch, f);

    // read the image size
    char buff[8];
    char *values = fgets(buff, 8, f);
    if (values != NULL) {
        if (sscanf(values, "%d %d", &im->width, &im->height) != 2) {
            printf("Error, the image size was not found.\n");
            exit(0);
        }
    }

    // validate the image size
    if (im->width <= 0 || im->height <= 0) {
        printf("Error, the image size is invalid.\n");
        exit(0);
    }

    // reset buff
    memset(buff, 0, sizeof(buff));

    // read the max rgb value
    values = fgets(buff, 8, f);
    if (values != NULL) {
        // if there's more than one value on this line
        // then we assume that this isn't the max rgb value and may be
        // the actual rgb values of each pixel
        if (sscanf(values, "%d", &im->max) != 1) {
            printf("Error, the maximum RGB value was not found.\n");
            exit(0);
        }
    }

    if (im->max < 0 || im->max > 255) {
        fprintf(stderr, "Error, the maximum RGB value is invalid.\n");
        exit(0);
    }

    // allocate memory to the pixels pointer
    im->pixels = (struct PPMPixel *)malloc(im->width * im->height * sizeof(struct PPMPixel));
    if (!im->pixels) {
        fprintf(stderr, "Unable to allocate memory to pixels pointer.\n");
        exit(0);
    }

    // store all the pixels
    int c, r, g, b;
    int i = 0;
    int imgSize = im->width * im->height;
    // For each pixel
    while (i < imgSize) {
        // While the next character is not EOF
        if ((c = fgetc(f)) != EOF) {
            /* Return the character */
            ungetc(c, f);
            // Grab the next three integers and assign them to r, g, b
            if (fscanf(f, "%d%d%d", &r, &g, &b) == 3) {
                im->pixels[i].red = r;
                im->pixels[i].green = g;
                im->pixels[i].blue = b;
            }
            i++;
        }
    }
    fclose(f);
    return im;
}

void showPPM(struct PPM *im) {
    printf("P3\n"); // print the P3 header
    printf("%s", im->comments); // print all comments
    printf("%d %d\n%d\n", im->width, im->height, im->max); // print the image size and max rgb value
    // display each pixel
    for (int i = 0; i < (im->width * im->height); i++) {
        if ((i % im->width == 0) && i > 0) {
            printf("\n");
        }
        printf("%d %d %d ", im->pixels[i].red, im->pixels[i].green, im->pixels[i].blue);
    }

    printf("\n");
}

struct PPM * encode(struct PPM * im, char * message, unsigned int mSize, unsigned int secret) {

    int imgSize = im->height * im->width;
    int length = strlen(message); // length of the message
    if (imgSize < length * 3) { // need at least 3 pixels per character to encode
        return NULL;
    }

    int msgCount = 0; // number of characters that have been encoded
    int pixelPos[length - 1]; // array to store pixels that have been encoded
    int numPixels = 0; // number of pixels that have been encoded
    int bitsCount; // number of bits that need to be encoded
    char c;
    int charBits[7];
    
    // first seed random number generator
    srand(secret);

    while (msgCount < length) {

        bitsCount = 7;
        c = message[msgCount];

        // put the ascii code for c in charBits
        for (int i = 7; i >= 0; i--) {
            charBits[i] = ( c >> i ) & 1 ? 1 : 0;
        }

        while (bitsCount >= 0) {

            // pick random pixel
            int pixelChoice = (rand() % imgSize);
            // if that pixel has been picked before
            // then pick new pixel
            while (containsElement(pixelChoice, pixelPos, numPixels)) {
                pixelChoice = (rand() % imgSize);
            }
            // add the pixel choice to the array of prev chosen pixels
            pixelPos[numPixels] = pixelChoice;
            // increment the num of pixels
            numPixels++;

            if (bitsCount >= 0) {
                printf("R VAL: %d BIT: %d PIXEL: %d BC: %d\n", im->pixels[pixelChoice].red, charBits[bitsCount], pixelChoice, bitsCount);
                if (!isSame(im->pixels[pixelChoice].red, charBits[bitsCount])) {
                    im->pixels[pixelChoice].red = im->pixels[pixelChoice].red + 1;
                    printf("R VAL: %d BIT: %d PIXEL: %d BC: %d -- Change\n", im->pixels[pixelChoice].red, charBits[bitsCount], pixelChoice, bitsCount);
                }
                bitsCount = bitsCount - 1;
            }
            //printf("R VAL: %d BIT: %d PIXEL: %d BC: %d\n", im->pixels[pixelChoice].red, charBits[bitsCount], pixelChoice, bitsCount);
            
            if (bitsCount >= 0) {
                printf("G VAL: %d BIT: %d PIXEL: %d BC: %d\n", im->pixels[pixelChoice].green, charBits[bitsCount], pixelChoice, bitsCount);
                if (!isSame(im->pixels[pixelChoice].green, charBits[bitsCount])) {
                    im->pixels[pixelChoice].green = im->pixels[pixelChoice].green + 1;
                    printf("G VAL: %d BIT: %d PIXEL: %d BC: %d -- Change\n", im->pixels[pixelChoice].green, charBits[bitsCount], pixelChoice, bitsCount);
                }
                bitsCount = bitsCount - 1;
            } 
            //printf("G VAL: %d BIT: %d PIXEL: %d BC: %d\n", im->pixels[pixelChoice].green, charBits[bitsCount], pixelChoice, bitsCount);

            if (bitsCount >= 0) {
                printf("B VAL: %d BIT: %d PIXEL: %d BC: %d\n", im->pixels[pixelChoice].blue, charBits[bitsCount], pixelChoice, bitsCount);
                if (!isSame(im->pixels[pixelChoice].blue, charBits[bitsCount])) {
                    im->pixels[pixelChoice].blue = im->pixels[pixelChoice].blue + 1;
                    printf("B VAL: %d BIT: %d PIXEL: %d BC: %d -- Change\n", im->pixels[pixelChoice].blue, charBits[bitsCount], pixelChoice, bitsCount);
                }
                bitsCount = bitsCount - 1;
            }
            //printf("B VAL: %d BIT: %d PIXEL: %d BC: %d\n", im->pixels[pixelChoice].blue, charBits[bitsCount], pixelChoice, bitsCount);
        }

        msgCount++;

    }

    return im;
    
}

char * decode(struct PPM * im, unsigned int secret) {

}

int main(int argc, char ** argv) {

    if (argc != 2) {
        printf("Not enough args.\n");
        exit(0);
    }

    FILE *f = fopen(argv[1], "r");
    struct PPM *im = getPPM(f);
    showPPM(im);

    char * p = "xaa";
    printf("\n");

    encode(im, p, 1, 123);

    return 0;
}
