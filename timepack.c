#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#define MAX_KEYPOINTS 1024
//#define DO_RANGES

static void usage(const char *name) {
	printf("Usage: %s infile outfile\n", name);
}

typedef struct {
	int tick;
	int signal;
	float value;
} Keypoint;

static int keypointCompare(const void *l, const void *r) {
	return ((const Keypoint*)l)->tick - ((const Keypoint*)r)->tick;
}

static struct {
	int ticks_per_bar;
	int num_keypoints;
	Keypoint keypoints[MAX_KEYPOINTS];
} data;

int timelineRead(FILE *f) {
	data.num_keypoints = 0;

	if (1 != fscanf(f, "%d\n", &data.ticks_per_bar)) {
		printf("Cannot read ticks per bar\n");
		return 0;
	}

	int line = 0;
	int prev_tick = 0;
	while (!feof(f)) {
		++line;

		if (data.num_keypoints == MAX_KEYPOINTS) {
			printf("Max keypoints %d reached\n", MAX_KEYPOINTS);
			return 0;
		}

		Keypoint *k = data.keypoints + data.num_keypoints;
		int bar, tick;
		char mode;
		k->tick = 0;
		const int rd = fscanf(f, "%c %d:%d %d %f\n", &mode, &bar, &tick, &k->signal, &k->value);
		if (5 == rd) {
			tick += bar * data.ticks_per_bar;
			switch (mode) {
			case 't':
				prev_tick = tick;
				k->tick = prev_tick;
				break;
			case 'd':
				prev_tick += tick;
				k->tick = prev_tick;
				break;
			default:
				printf("Invalid time mode %c\n", mode);
				return 0;
			}
		} else {
			printf("Line %d has invalid format\n", line);
			return 0;
		}

		++data.num_keypoints;
	}

	qsort(data.keypoints, data.num_keypoints, sizeof(Keypoint), keypointCompare);

	printf("Read %d lines, %d keypoints\n", line, data.num_keypoints);
	return 1;
}

int timelineWrite(FILE *f) {
	fprintf(f, "/* AUTOGENERATED BY " __FILE__ ", DO NOT MODIFY */\n");
	fprintf(f, "#define TIMELINE_KEYPOINTS (%d)\n", data.num_keypoints);
	fprintf(f, "#pragma data_seg(\".timeline_data\")\n");
#ifdef DO_RANGES
	fprintf(f, "static const unsigned char timeline_ranges[%d] = {\n\t", data.columns);
	for (int i = 0; i < data.columns; ++i) {
		const float r = data.column_ranges[i];
		if (r > 255.f) {
			printf("Error at col %d: range is too big: %f > 255", i + 1, r);
			return 0;
		}
		data.column_ranges[i] = ceilf(r);
		fprintf(f, "%d%s", (int)(data.column_ranges[i]), (i != (data.columns-1)) ? ", " : "};\n");
	}
#endif
	fprintf(f, "static const unsigned char timeline_times[%d] = {\n\t", data.num_keypoints);
	int prevtime = 0;
	for (int i = 0; i < data.num_keypoints; ++i) {
		const Keypoint *k = data.keypoints + i;
		const int deltatime = k->tick - prevtime;
		if (deltatime > 255) {
			printf("Error at row %d: dt=%d, but it should be less than %d", i + 1, deltatime, 255);
			return 0;
		}
		if (deltatime < 0) {
			printf("Error at row %d: dt=%d, but it should be at least %d", i + 1, deltatime, 0);
			return 0;
		}
		fprintf(f, "%d%s", deltatime, (i != (data.num_keypoints - 1)) ? ", " : "\n};\n");
		prevtime = k->tick;
	}

	fprintf(f, "static const unsigned char timeline_signals[%d] = {\n\t", data.num_keypoints);
	for (int i = 0; i < data.num_keypoints; ++i) {
		const Keypoint *k = data.keypoints + i;
		fprintf(f, "%d%s", k->signal, (i != (data.num_keypoints - 1)) ? ", " : "\n};\n");
		prevtime = k->tick;
	}

	fprintf(f, "static const unsigned char timeline_values[%d] = {\n\t", data.num_keypoints);
	for (int i = 0; i < data.num_keypoints; ++i) {
		const Keypoint *k = data.keypoints + i;
		int value = k->value * 255.f / 32.f; //* 255.f / (2.f * 67.5f);
		if (value < 0) { printf("WARN: %d %f -> %d\n", i, k->value, value); value = 0; }
		if (value > 255) { printf("WARN: %d %f -> %d\n", i, k->value, value); value = 255; }
		fprintf(f, "%d%s", value, (i != (data.num_keypoints - 1)) ? ", " : "\n};\n");
		prevtime = k->tick;
	}
	return 1;
}

int main(int argc, char *argv[]) {
	FILE *fin, *fout;

	if (argc < 3) {
		usage(argv[0]);
		return 1;
	}

	fin = fopen(argv[1], "r");
	if (!fin) {
		printf("cannot open file %s\n", argv[1]);
		return 1;
	}

	if (!timelineRead(fin)) {
		printf("Unable to parse file %s\n", argv[1]);
		return 1;
	}

	fclose(fin);

	fout = fopen(argv[2], "w");
	if (!fin) {
		printf("cannot open file %s\n", argv[2]);
		return 1;
	}

	if (!timelineWrite(fout)) {
		printf("Unable to write file %s\n", argv[2]);
		return 1;
	}

	fclose(fout);

	return 0;
}
