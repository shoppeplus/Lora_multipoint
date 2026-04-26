#include "fft_analysis.h"

float vReal[SAMPLE_SIZE];
float vImag[SAMPLE_SIZE];
float hannWindow[SAMPLE_SIZE];

void initHannWindow() {
    for (int i = 0; i < SAMPLE_SIZE; i++) {
        hannWindow[i] = 0.5f * (1.0f - cosf(2.0f * PI * i / (SAMPLE_SIZE - 1)));
    }
}

static void fft(float* real, float* imag, int n) {
    int j = 0;
    for (int i = 1; i < n - 1; i++) {
        int bit = n >> 1;
        while (j & bit) { j ^= bit; bit >>= 1; }
        j ^= bit;
        if (i < j) {
            float temp = real[i]; real[i] = real[j]; real[j] = temp;
            temp = imag[i]; imag[i] = imag[j]; imag[j] = temp;
        }
    }

    for (int len = 2; len <= n; len <<= 1) {
        float angle = -2.0f * PI / len;
        float wReal = cosf(angle);
        float wImag = sinf(angle);
        for (int i = 0; i < n; i += len) {
            float curReal = 1.0f, curImag = 0.0f;
            for (int k = 0; k < len / 2; k++) {
                int idx1 = i + k;
                int idx2 = i + k + len / 2;
                float tReal = curReal * real[idx2] - curImag * imag[idx2];
                float tImag = curReal * imag[idx2] + curImag * real[idx2];
                real[idx2] = real[idx1] - tReal;
                imag[idx2] = imag[idx1] - tImag;
                real[idx1] += tReal;
                imag[idx1] += tImag;
                float newCurReal = curReal * wReal - curImag * wImag;
                curImag = curReal * wImag + curImag * wReal;
                curReal = newCurReal;
            }
        }
    }
}

void processAxisFFT(float* axisSamples, float* outFreqs, int numFreqs,
                    float* outRms, float* outPeak, float* outCF) {
    // 1. DC removal (trừ gravity)
    float mean = 0;
    for (int i = 0; i < SAMPLE_SIZE; i++) mean += axisSamples[i];
    mean /= SAMPLE_SIZE;

    // 2. Peak + RMS (sau DC removal)
    float rmsVal = 0;
    float peakVal = 0;
    for (int i = 0; i < SAMPLE_SIZE; i++) {
        axisSamples[i] -= mean;
        float absVal = fabsf(axisSamples[i]);
        if (absVal > peakVal) peakVal = absVal;
        rmsVal += axisSamples[i] * axisSamples[i];
    }
    rmsVal = sqrtf(rmsVal / SAMPLE_SIZE);
    *outRms = rmsVal;
    *outPeak = peakVal;

    // 3. Crest Factor
    if (outCF != NULL) {
        *outCF = (rmsVal > 0.0001f) ? (peakVal / rmsVal) : 0;
    }

    // 4. Hann window + FFT
    for (int i = 0; i < SAMPLE_SIZE; i++) {
        vReal[i] = axisSamples[i] * hannWindow[i];
        vImag[i] = 0;
    }
    fft(vReal, vImag, SAMPLE_SIZE);

    // 5. Magnitude spectrum
    int halfSize = SAMPLE_SIZE / 2;
    for (int i = 0; i < halfSize; i++) {
        vReal[i] = sqrtf(vReal[i] * vReal[i] + vImag[i] * vImag[i]);
    }

    // 6. Peak finding (noise floor)
    int freqBins[NUM_TOP_FREQS];
    for (int p = 0; p < numFreqs; p++) {
        float maxMag = 0;
        int maxIdx = 0;

        for (int i = 1; i < halfSize; i++) {
            if (vReal[i] > maxMag) {
                bool tooClose = false;
                for (int q = 0; q < p; q++) {
                    if (abs(i - freqBins[q]) < MIN_PEAK_DISTANCE) {
                        tooClose = true;
                        break;
                    }
                }
                if (!tooClose) {
                    maxMag = vReal[i];
                    maxIdx = i;
                }
            }
        }

        if (maxMag >= NOISE_FLOOR_MAG) {
            outFreqs[p] = maxIdx * ((float)SAMPLE_RATE / SAMPLE_SIZE);
        } else {
            outFreqs[p] = 0;
        }
        freqBins[p] = maxIdx;
    }
}
