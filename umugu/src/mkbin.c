#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv)
{
    if (argc != 2) {
        printf("Usage: [exec name] [output filename]\n");
        printf("\te.g. mkbin ./assets/pipelines/example.bin\n");
        return 1;
    }

    FILE *f = fopen(argv[1], "wb");
    if (!f) {
        printf("Could not open the file %s for writing.\n", argv[1]);
        return 1;
    }

    fwrite("IEE", 4, 1, f);

    int version = 1 << 10;
    fwrite(&version , 4, 1, f);

    size_t pipeline_size = 272;
    fwrite(&pipeline_size, 8, 1, f);

    char limiter_name[32] = "Limiter";
    fwrite(limiter_name, 32, 1, f);

    float minmax[2] = {-1.0f, 1.0f};
    fwrite(minmax, 8, 1, f);

    char ampl_name[32] = "Amplitude";
    fwrite(ampl_name, 32, 1, f);
    float multi = 1.0f;
    fwrite(&multi, 4, 1, f);
    multi = 0;
    fwrite(&multi, 4, 1, f); // Padding 

    char inspec_name[32] = "Inspector";
    fwrite(inspec_name, 32, 1, f);
    int64_t padd[5] = {-1, -1, -1, -1, -1};
    fwrite(&padd, 5 * sizeof(int64_t), 1, f);

    char wav_name[32] = "WavFilePlayer";
    fwrite(wav_name, 32, 1, f);

    struct {
        void *ptr;
        size_t cap;
    } samples;

    samples.ptr = NULL;
    samples.cap = 1024;

    fwrite(&samples, 16, 1, f);

    char filename[64] = "../assets/audio/littlewing.wav";
    fwrite(filename, 64, 1, f);
    
    fwrite(&samples.ptr, 8, 1, f);

    fwrite("AUU", 4, 1, f);

    fclose(f);

    return 0;
}
