//norm
#include <iostream>


//net
#include <windows.h>

//openal
#include "../lib/include/al.h"
#include "../lib/include/alc.h"

//thread
#include <thread>

//net lib 
#pragma comment(lib, "ws2_32.lib")

//openal lib
#pragma comment(lib,"../lib/Win64/OpenAL32.lib")

//def
//#define CLPORT 5666
#define FREQ 22050
#define BUF_LEN 4410

//
using std::cout;
using std::endl;

int opow(int a) {
	int re = 1;
	for (int i = 0; i < a; ++i) {
		re = 10 * re;
	}
	return re;
}


int ctoi(char* cint) {
	unsigned i = strlen(cint), iit = 0;
	for (int j = i; j > 0; --j) {
		//cout << (int)cint[i - j] - 48 << endl;
		iit = iit + ((int)(cint[i - j]) - 48) * opow(j - 1);
	}
	//cout << (int)cint[0] << endl;
	return iit;
}

int capture_send_data(ALCdevice* rdevice, ALint* sample, char* data, SOCKET* scli) {
	while (true) {
		Sleep(1);
		alcGetIntegerv(rdevice, ALC_CAPTURE_SAMPLES, 1, sample);
		if (*sample > BUF_LEN / 2) {
			alcCaptureSamples(rdevice, data, BUF_LEN / 2);
			send(*scli, data, BUF_LEN, 0);

		}
	}
	return 0;

}


int recv_play_data(ALuint* asource, SOCKET* scli, char* data) {

	ALint istate, istate1, istate2;
	ALuint buf;
	while (true) {
		Sleep(1);//rel cpu time

		alGetSourcei(*asource, AL_BUFFERS_PROCESSED, &istate);
		if (istate <= 0) {
			continue;
		}

		if (recv(*scli, data, BUF_LEN, 0) <= 0) {
			cout << "connection closed " << endl;
			return 0;
		}

		while (istate > 0) {
			alSourceUnqueueBuffers(*asource, 1, &buf);
			istate--;
		}
		alBufferData(buf, AL_FORMAT_MONO16, data, BUF_LEN, FREQ);
		alSourceQueueBuffers(*asource, 1, &buf);

		alGetSourcei(*asource, AL_SOURCE_STATE, &istate1);
		if (istate != AL_PLAYING) {
			alGetSourcei(*asource, AL_BUFFERS_QUEUED, &istate2);
			if (istate2 != 0) {
				alSourcePlay(*asource);
			}
		}
	}
	alDeleteBuffers(1, &buf);

	return 0;
}

int main(int argc, char** argv) {

	//var

	int rport = ctoi(argv[2]);
	//init net 

	WORD sv = MAKEWORD(2, 2);
	WSADATA wsadata;
	if (WSAStartup(sv, &wsadata)) {
		cout << "startup failed" << endl;
		return 1;

	}

	SOCKET scli = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (scli == INVALID_SOCKET) {
		cout << "init sock error" << endl;
		return 1;
	}

	struct sockaddr_in cliaddr;
	cliaddr.sin_family = AF_INET;
	cliaddr.sin_port = htons(rport);
	cliaddr.sin_addr.s_addr = inet_addr(argv[1]);

	if (connect(scli, (sockaddr*)&cliaddr, sizeof(cliaddr)) < 0) {
		cout << "connect error" << endl;
		return 1;
	}


	//init openal
	char recv_data[BUF_LEN + 1] = { 0 },
		send_data[BUF_LEN] = { 0 };
	ALCdevice* pdevice = alcOpenDevice(nullptr),
		* rdevice = alcCaptureOpenDevice(nullptr, FREQ, AL_FORMAT_MONO16, BUF_LEN);
	ALCcontext* ctx;
	ALuint asource, ibuffer[4];
	ALint sample;

	ctx = alcCreateContext(pdevice, nullptr);
	alcMakeContextCurrent(ctx);
	alGenSources(1, &asource);
	alGenBuffers(4, ibuffer);
	//inited


	//recv and set head buffer
	if (recv(scli, recv_data, sizeof(recv_data) - 1, 0) <= 0) {
		cout << "head buffer error" << endl;
		return 1;
	}
	alBufferData(ibuffer[0], AL_FORMAT_MONO16, recv_data, sizeof(recv_data), FREQ);
	alBufferData(ibuffer[1], AL_FORMAT_MONO16, recv_data, sizeof(recv_data), FREQ);
	alBufferData(ibuffer[2], AL_FORMAT_MONO16, recv_data, sizeof(recv_data), FREQ);
	alBufferData(ibuffer[3], AL_FORMAT_MONO16, recv_data, sizeof(recv_data), FREQ);
	alSourceQueueBuffers(asource, 4, ibuffer);

	//alDistanceModel(AL_NONE);

	//play first time
	alSourcePlay(asource);

	//record
	alcCaptureStart(rdevice);

	std::thread thr(recv_play_data, &asource, &scli, recv_data);
	std::thread ths(capture_send_data, rdevice, &sample, send_data, &scli);
	cout << "running..." << endl;
	thr.join();

	//recv capturebuffer and play
	//while (true) {

	//	Sleep(1);//rel cpu time

	//	alGetSourcei(asource, AL_BUFFERS_PROCESSED, &istate);
	//	if (istate <= 0) {
	//		continue;
	//	}

	//	//recv buffer data
	//	recv(scli, data, sizeof(data) - 1, 0);
	//	//cout << (int)data[0] << (int)data[1] << (int)data[2] << (int)data[3] << (int)data[4] << (int)data[5] << (int)data[6] << endl;

	//	
	//	//play received buffer
	//	
	//	while (istate > 0) {
	//		alSourceUnqueueBuffers(asource, 1, &buf);
	//		istate--;
	//	}
	//	
	//	alBufferData(buf, AL_FORMAT_MONO16, data, 4410, FREQ);
	//	alSourceQueueBuffers(asource, 1, &buf);

	//	alGetSourcei(asource, AL_SOURCE_STATE, &istate1);

	//	if (istate1 != AL_PLAYING) {
	//		alGetSourcei(asource, AL_BUFFERS_QUEUED, &istate2);
	//		if (istate2 != 0) {
	//			alSourcePlay(asource);

	//		}
	//	}

	//	



	//}

	alSourceStop(asource);
	alDeleteSources(1, &asource);
	alDeleteBuffers(4, ibuffer);
	//alDeleteBuffers(1, &buf);

	alcMakeContextCurrent(NULL);
	alcDestroyContext(ctx);
	alcCloseDevice(pdevice);
	alcCaptureStop(rdevice);
	alcCaptureCloseDevice(rdevice);
	closesocket(scli);

	WSACleanup();

	return 0;
}
