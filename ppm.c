#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

int commentsLength = 100; // global to store the size of the comments pointer

struct PPMPixel {
    // rgb[0] is the red value of the pixel, rgb[1] is green and rgb[2] is blue
    unsigned int rgb[3]; 
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

    int i;
    for (i = 0; i < len; i++) {
        if (arr[i] == a) return 1;
    }

    return 0;
}

// return true when i is even
int isEven(int i) { return (i % 2 == 0) ? 1 : 0; }

// return true when i is odd
int isOdd(int i) { return (i % 2 == 1) ? 1 : 0; }

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

    // append c to the comments pointer
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
    int r, g, b;
    int i = 0;
    int imgSize = im->width * im->height;
    // For each pixel
    while (i < imgSize) {
        // While the next character is not EOF
        if ((ch = fgetc(f)) != EOF) {
            // Return the character
            ungetc(ch, f);
            // Grab the next three integers and assign them to r, g, b
            if (fscanf(f, "%d%d%d", &r, &g, &b) == 3) {
                im->pixels[i].rgb[0] = r;
                im->pixels[i].rgb[1] = g;
                im->pixels[i].rgb[2] = b;
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
    int i;
    for (i = 0; i < (im->width * im->height); i++) {
        if ((i % im->width == 0) && i > 0) {
            printf("\n");
        }
        printf("%d %d %d ", im->pixels[i].rgb[0], im->pixels[i].rgb[1], im->pixels[i].rgb[2]);
    }
}

struct PPM * encode(struct PPM * im, char * message, unsigned int mSize, unsigned int secret) {

    int imgSize = im->height * im->width;
    if (imgSize < mSize * 3) { // if there isn't enough pixels to encode the message then return null
        return NULL; 
    }

    int msgCount = 0; // number of characters that have been encoded
    int pixelPos[imgSize]; // array to store pixels that have been encoded
    int numPixels = 0; // number of pixels that have been encoded
    int bitsCount; // number of bits that need to be encoded
    char c;
    int charBits[7]; // stores the ascii code of a character 
    int pixelChoice;
    
    // seed the random number generator
    srand(secret);

    while (msgCount < mSize) {

        bitsCount = 7;
        c = message[msgCount]; // get the character from the message 
        
        // put the ascii code for c in charBits
        int i;
        for (i = 7; i >= 0; i--) {
            charBits[i] = ( c >> i ) & 1 ? 1 : 0;
        }

        while (bitsCount >= 0) {

            // pick random pixel
            pixelChoice = rand() % imgSize;
            // if that pixel has been picked before then pick new pixel
            while (containsElement(pixelChoice, pixelPos, numPixels)) {
                pixelChoice = rand() % imgSize;
            }
            // add the pixel choice to the array of prev chosen pixels
            pixelPos[numPixels] = pixelChoice;
            // increment the num of pixels
            numPixels++;

            // loop through the rgb values of each pixel
            int j;
            for (j = 0; j <= 2; j++) {
                if (bitsCount >= 0) {
                    //printf("RGB: %d PIXEL VAL: %d BIT: %d PIXEL: %d BC: %d\n", i, im->pixels[pixelChoice].rgb[i], charBits[bitsCount], pixelChoice, bitsCount);
                    if (!isSame(im->pixels[pixelChoice].rgb[j], charBits[bitsCount])) {
                        im->pixels[pixelChoice].rgb[j]++;
                        //printf("RGB: %d PIXEL VAL: %d BIT: %d PIXEL: %d BC: %d -- Change\n", i, im->pixels[pixelChoice].rgb[i], charBits[bitsCount], pixelChoice, bitsCount);
                    }
                    bitsCount--;
                }
            }
        }
        msgCount++; // increment to the next character
    }
    return im;
}

char * decode(struct PPM * im, unsigned int secret) {

    int imgSize = im->height * im->width;
    int length = floor((imgSize / 3)); // length of the message
  
    int msgCount = 0; // number of characters that have been encoded
    int pixelPos[imgSize]; // array to store pixels that have been encoded
    int numPixels = 0; // number of pixels that have been encoded
    int bitsCount; // number of bits that need to be encoded
    char c;
    char bit;
    int pixelChoice;
    char * charBits = malloc(7 * sizeof(char));
    char * message = malloc(length * sizeof(char));
    
    // seed the random number generator
    srand(secret);

    while (msgCount < length) {

        bitsCount = 7;
        while (bitsCount >= 0) {

            // pick random pixel
            pixelChoice = rand() % imgSize;
            // if that pixel has been picked before then pick new pixel
            while (containsElement(pixelChoice, pixelPos, numPixels)) {
                pixelChoice = rand() % imgSize;
            }
            // add the pixel choice to the array of prev chosen pixels
            pixelPos[numPixels] = pixelChoice;
            // increment the num of pixels
            numPixels++;

            // loop through the rgb values of each pixel
            int i;
            for (i = 0; i <= 2; i++) {
                if (bitsCount >= 0) {
                    //printf("BIT: %d BITSCOUNT: %d CHAR: %c PIXELCHOICE: %d\n", isOdd(im->pixels[pixelChoice].rgb[i]), bitsCount, (char)(isOdd(im->pixels[pixelChoice].rgb[i]) + '0'), pixelChoice);
                    bit = isOdd(im->pixels[pixelChoice].rgb[i]) + '0'; // convert bit integer to character
                    // bit come in from least to most significant figures 
                    // but the array needs to store them from most to least  
                    charBits[7 - bitsCount] = bit;
                    if (bitsCount == 0) {
                        c = strtol(charBits, (char**)NULL, 2); // convert the binary string charBits to a character
                        //printf("CHAR: %c MSGCOUNT: %d\n", c, msgCount);
                        message[msgCount] = c;
                        memset(charBits, 0, sizeof(charBits));
                    }
                    bitsCount--;
                }
            }    
        }
        msgCount++; // increment to the next character
    }
    message[msgCount] = '\0';
    return message;
}

int main(int argc, char ** argv) {

    unsigned int secret;
    FILE *f;
    
    if (argv[1] != NULL) {
        if (!strcmp("e", argv[1])) {
            if (argv[2] != NULL) {
                char encodedMessage[100];
                // get the message and secret value
                scanf("%[^\n]%*c", encodedMessage);
                scanf("%d", &secret);
                // open the file, create the ppm object and encode the message into it
                f = fopen(argv[2], "r");
                struct PPM *im = getPPM(f);
                im = encode(im, encodedMessage, strlen(encodedMessage), secret);
                showPPM(im);
            }
        } else if (!strcmp("d", argv[1])) {
            if (argv[2] != NULL) {
                scanf("%d", &secret);
                f = fopen(argv[2], "r");
                struct PPM *im = getPPM(f);
                printf("%s\n", decode(im, secret));
            }
        }
    }
    return 0;
}
