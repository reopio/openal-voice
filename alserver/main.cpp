//normal
#include <iostream>


//net
#include <windows.h>


//openal
#include "../openal-lib/include/al.h"
#include "../openal-lib/include/alc.h"


//thread
#include <thread>

//net lib
#pragma comment(lib, "ws2_32.lib")

//openal lib 
#pragma comment(lib,"../openal-lib/Win64/OpenAL32.lib")

//def
#define SEPORT 5666
#define FREQ 22050
#define BUF_LEN 4410



//
using std::cout;
using std::endl;

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


	return 0;
}


int main() {


	//var
	SOCKET scli;



	//init
	WORD sv = MAKEWORD(2, 2);
	WSADATA wsadata;
	if (WSAStartup(sv, &wsadata)) {
		cout << "socket start failed" << endl;
		return 1;
	}

	//create socket
	SOCKET slisten = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (slisten == INVALID_SOCKET) {
		cout << "init socket error" << endl;
		return 1;
	}

	//init ip&port
	struct sockaddr_in sin;
	sin.sin_family = AF_INET;
	sin.sin_port = htons(SEPORT);
	sin.sin_addr.s_addr = htonl(INADDR_ANY);

	if (bind(slisten, (sockaddr*)&sin, sizeof(sin)) < 0) {
		cout << "bind error" << endl;
		return 1;
	}

	if (listen(slisten, 20) == -1) {
		cout << "listen error" << endl;
		return 1;
	}

	cout << "wait for request..." << endl;

	if ((scli = accept(slisten, (sockaddr*)NULL, NULL)) < 0) {
		cout << "accept error" << endl;
		return 1;

	}
	//cout << "sending audio data..." << endl;

	//openal init
	char send_data[BUF_LEN] = { 0 },
		recv_data[BUF_LEN + 1] = { 0 };
	ALCdevice* rdevice = alcCaptureOpenDevice(nullptr, FREQ, AL_FORMAT_MONO16, BUF_LEN),
		* pdevice = alcOpenDevice(nullptr);
	ALCcontext* ctx;
	ALuint asource, ibuffer[4];
	ALint sample;

	ctx = alcCreateContext(pdevice, nullptr);
	alcMakeContextCurrent(ctx);
	alGenSources(1, &asource);
	alGenBuffers(4, ibuffer);

	//send head buffer and fill head buffer
	send(scli, send_data, sizeof(send_data), 0);
	alBufferData(ibuffer[0], AL_FORMAT_MONO16, send_data, sizeof(send_data), FREQ);
	alBufferData(ibuffer[1], AL_FORMAT_MONO16, send_data, sizeof(send_data), FREQ);
	alBufferData(ibuffer[2], AL_FORMAT_MONO16, send_data, sizeof(send_data), FREQ);
	alBufferData(ibuffer[3], AL_FORMAT_MONO16, send_data, sizeof(send_data), FREQ);
	alSourceQueueBuffers(asource, 4, ibuffer);


	//play head buffer
	alSourcePlay(asource);

	//alDistanceModel(AL_NONE);

	//capture and send
	alcCaptureStart(rdevice);

	//while (true) {

	//	Sleep(50);//release cpu time

	//	alcGetIntegerv(rdevice, ALC_CAPTURE_SAMPLES, 1, &sample);
	//	alcCaptureSamples(rdevice, data, sample);
	//	//cout << (int)data[0] << (int)data[1] << (int)data[2] << (int)data[3] << (int)data[4] << (int)data[5] << (int)data[6] << endl;
	//	send(scli, data, sizeof(data), 0);

	//}


	std::thread ths(capture_send_data, rdevice, &sample, send_data, &scli);
	std::thread thr(recv_play_data, &asource, &scli, recv_data);
	cout << "runing..." << endl;
	thr.join();


	alSourceStop(asource);
	alDeleteSources(1, &asource);
	alDeleteBuffers(4, ibuffer);
	//alDeleteBuffers(1, &buf);

	alcMakeContextCurrent(NULL);
	alcDestroyContext(ctx);

	alcCaptureStop(rdevice);
	alcCaptureCloseDevice(rdevice);
	alcCloseDevice(pdevice);
	closesocket(slisten);

	//alcCloseDevice(pdevice);

	WSACleanup();



	return 0;
}