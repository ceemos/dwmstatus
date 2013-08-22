/* made by profil 2011-12-29.
**
** Compile with:
** gcc -Wall -pedantic -std=c99 -lX11 status.c
*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <inttypes.h>
#include <time.h>
#include <X11/Xlib.h>

static Display *dpy;

void setstatus(char *str) {
	XStoreName(dpy, DefaultRootWindow(dpy), str);
	XSync(dpy, False);
}

float getfreq(char *file) {
	FILE *fd;
	char *freq; 
	float ret;

	freq = malloc(10);
	fd = fopen(file, "r");
	if(fd == NULL) {
		fprintf(stderr, "Cannot open '%s' for reading.\n", file);
		exit(1);
	}

	fgets(freq, 10, fd);
	fclose(fd);

	ret = atof(freq)/1000000;
	free(freq);
	return ret;
}

char *getdatetime() {
	static char buf[64];
	time_t result;
	struct tm *resulttm;

	result = time(NULL);
	resulttm = localtime(&result);
	if(resulttm == NULL) {
		fprintf(stderr, "Error getting localtime.\n");
		exit(1);
	}
	if(!strftime(buf, sizeof(char)*64-1, "%a %b %d %H:%M:%S", resulttm)) {
		fprintf(stderr, "strftime is 0.\n");
		exit(1);
	}
	
	return buf;
}

int getbattery() {
	FILE *fd;
	int energy_now, energy_full;

	fd = fopen("/sys/class/power_supply/BAT0/charge_now", "r");
	if(fd == NULL) {
		fprintf(stderr, "Error opening charge_now.\n");
		return -1;
	}
	fscanf(fd, "%d", &energy_now);
	fclose(fd);


	fd = fopen("/sys/class/power_supply/BAT0/charge_full", "r");
	if(fd == NULL) {
		fprintf(stderr, "Error opening charge_full.\n");
		return -1;
	}
	fscanf(fd, "%d", &energy_full);
	fclose(fd);

	return (int)((float) energy_now / (float) energy_full * 100.0);
}


float getpower() {
	FILE *fd;
	int voltage_now, current_now;

	fd = fopen("/sys/class/power_supply/BAT0/voltage_now", "r");
	if(fd == NULL) {
		fprintf(stderr, "Error opening voltage_now.\n");
		return -1;
	}
	fscanf(fd, "%d", &voltage_now);
	fclose(fd);
	
	fd = fopen("/sys/class/power_supply/BAT0/current_now", "r");
	if(fd == NULL) {
		fprintf(stderr, "Error opening current_now.\n");
		return -1;
	}
	fscanf(fd, "%d", &current_now);
	fclose(fd);

	uint64_t pwr = voltage_now;
	pwr *= current_now;
	pwr /= 1000000000;
	return (float) pwr / 1000.0;
}


char* getnetusage() {
	static char buf[255];
	static char outbuf[255];
	char* outpos = outbuf;
	static uint64_t oldvalsrx[10];
	static uint64_t oldvalstx[10];
	uint64_t absrx, abstx;
	static char namebuf[16];
	int bufsize = 255;
	FILE *devfd;
	bufsize = 255;
	devfd = fopen("/proc/net/dev", "r");

	// ignore the first two lines of the file
	fgets(buf, bufsize, devfd);
	fgets(buf, bufsize, devfd);
	int i = 0;
	outbuf[0] = '\0';
	while (fgets(buf, bufsize, devfd)) {
		
		// With thanks to the conky project at http://conky.sourceforge.net/
		int hits = sscanf(buf, "%s %" SCNu64 "  %*d %*d  %*d %*d %*d %*d %*d %" SCNu64, namebuf, &absrx, &abstx);
		if (hits == 3 && (oldvalsrx[i] != absrx || oldvalstx[i] != abstx)) {
		  uint64_t relrx = absrx - oldvalsrx[i];
		  uint64_t reltx = abstx - oldvalstx[i];
		  double rx, tx;
		  char *rxc, *txc;
		  rxc = "kB/s"; txc = "kB/s";
		  rx = relrx / 1024.0;
		  tx = reltx / 1024.0;
		  if (rx > 1024.0) {
		    rxc = "MB/s"; rx /= 1024.0;
		  }
		  if (tx > 1024.0) {
		    txc = "MB/s"; tx /= 1024.0;
		  }
		  
		  outpos += sprintf(outpos, "%s %.2f %s rx, %.2f %s tx ", namebuf, rx, rxc, tx, txc);
		  oldvalsrx[i] = absrx;
		  oldvalstx[i] = abstx;
		}
		i++;
	}
	fclose(devfd);
	return outbuf;
}

int* getcpuload() {
	FILE *fd;
#define MAXCPU 2
	static long cpu_work[MAXCPU], cpu_total[MAXCPU];
	long jif1, jif2, jif3, jif4, jif5, jif6, jif7;
	long work[MAXCPU], total[MAXCPU];
	static 	int load[MAXCPU];

	fd = fopen("/proc/stat", "r");
	char c;
	while (c != '\n') c = fgetc(fd); // read first line
	for (int i = 0; i < MAXCPU; i++) {
		c = 0;
		fscanf(fd, "cpu%*d %ld %ld %ld %ld %ld %ld %ld", &jif1, &jif2, &jif3, &jif4, &jif5, &jif6, &jif7);
		while (c != '\n') c = fgetc(fd); // consume rest of line
		work[i] = jif1 + jif2 + jif3 + jif6 + jif7;
		total[i] = work[i] + jif4 + jif5;
		load[i] = 100 * (work[i] - cpu_work[i]) / (total[i] - cpu_total[i]);
		cpu_work[i] = work[i];
		cpu_total[i] = total[i];
	}

	fclose(fd);
	return load;

}

int main(void) {
	char *status;
	  float cpu0;
	char *datetime;
	int bat0;
	float pwr;
	char *net;
	int* load;


	if (!(dpy = XOpenDisplay(NULL))) {
		fprintf(stderr, "Cannot open display.\n");
		return 1;
	}

	if((status = malloc(200)) == NULL)
		exit(1);
	

	for (;;sleep(1)) {
		cpu0 = getfreq("/sys/devices/system/cpu/cpu0/cpufreq/scaling_cur_freq");
		datetime = getdatetime();
		bat0 = getbattery();
		pwr = getpower();
		net = getnetusage();
		load = getcpuload();
		
		snprintf(status, 200, "%s | %0.2f | %d%% %d%% | %d%% %0.2fW | %s", net, cpu0, load[0], load[1], bat0, pwr, datetime);

		setstatus(status);
		//fprintf(stderr, "%s\n", status);
	}

	free(status);
	XCloseDisplay(dpy);

	return 0;
}

