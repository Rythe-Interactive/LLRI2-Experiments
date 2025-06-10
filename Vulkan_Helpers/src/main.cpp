// Engine
#include "vk_engine.hpp"

// Macros to retrieve CMake variables
#define Q(x) #x
#define QUOTE(x) Q(x)

#ifdef NDEBUG
constexpr bool debug_mode = false;
#else
constexpr bool debug_mode = true;
#endif

VulkanEngine* vulkanEngine;

extern "C" {
#include <doomgeneric.h>
#include <doomkeys.h>

#define KEYQUEUE_SIZE 16
static unsigned short s_KeyQueue[KEYQUEUE_SIZE];
static unsigned int s_KeyQueueWriteIndex = 0;
static unsigned int s_KeyQueueReadIndex = 0;

static unsigned char convertToDoomKey(unsigned int key) {
	switch (key) {
		case SDLK_RETURN:
			key = KEY_ENTER;
			break;
		case SDLK_ESCAPE:
			key = KEY_ESCAPE;
			break;
		case SDLK_LEFT:
			key = KEY_LEFTARROW;
			break;
		case SDLK_RIGHT:
			key = KEY_RIGHTARROW;
			break;
		case SDLK_UP:
			key = KEY_UPARROW;
			break;
		case SDLK_DOWN:
			key = KEY_DOWNARROW;
			break;
		case SDLK_LCTRL:
		case SDLK_RCTRL:
			key = KEY_FIRE;
			break;
		case SDLK_SPACE:
			key = KEY_USE;
			break;
		case SDLK_LSHIFT:
		case SDLK_RSHIFT:
			key = KEY_RSHIFT;
			break;
		case SDLK_LALT:
		case SDLK_RALT:
			key = KEY_LALT;
			break;
		case SDLK_F2:
			key = KEY_F2;
			break;
		case SDLK_F3:
			key = KEY_F3;
			break;
		case SDLK_F4:
			key = KEY_F4;
			break;
		case SDLK_F5:
			key = KEY_F5;
			break;
		case SDLK_F6:
			key = KEY_F6;
			break;
		case SDLK_F7:
			key = KEY_F7;
			break;
		case SDLK_F8:
			key = KEY_F8;
			break;
		case SDLK_F9:
			key = KEY_F9;
			break;
		case SDLK_F10:
			key = KEY_F10;
			break;
		case SDLK_F11:
			key = KEY_F11;
			break;
		case SDLK_EQUALS:
		case SDLK_PLUS:
			key = KEY_EQUALS;
			break;
		case SDLK_MINUS:
			key = KEY_MINUS;
			break;
		default:
			key = tolower(key);
			break;
	}

	return key;
}

static void addKeyToQueue(int pressed, unsigned int keyCode) {
	unsigned char key = convertToDoomKey(keyCode);

	unsigned short keyData = (pressed << 8) | key;

	s_KeyQueue[s_KeyQueueWriteIndex] = keyData;
	s_KeyQueueWriteIndex++;
	s_KeyQueueWriteIndex %= KEYQUEUE_SIZE;
}

static void handleKeyInput() {
	SDL_Event event;
	while (SDL_PollEvent(&event)) {
		if (event.type == SDL_EVENT_QUIT) {
			puts("Quit requested");
			atexit(SDL_Quit);
			exit(1);
		}
		if (event.type == SDL_EVENT_KEY_DOWN) {
			//KeySym sym = XKeycodeToKeysym(s_Display, e.xkey.keycode, 0);
			//printf("KeyPress:%d sym:%d\n", e.xkey.keycode, sym);
			addKeyToQueue(1, event.key.key);
		} else if (event.type == SDL_EVENT_KEY_UP) {
			//KeySym sym = XKeycodeToKeysym(s_Display, e.xkey.keycode, 0);
			//printf("KeyRelease:%d sym:%d\n", e.xkey.keycode, sym);
			addKeyToQueue(0, event.key.key);
		}

		vulkanEngine->HandleEvent(&event);
	}
}

void DG_Init() {
	const std::string name = QUOTE(MYPROJECT_NAME);
	vulkanEngine = new VulkanEngine(name, debug_mode);
	vulkanEngine->Init(1280, 720);
}

void DG_DrawFrame() {
	vulkanEngine->SetScreenBuffer(DG_ScreenBuffer, sizeof(uint32_t), DOOMGENERIC_RESX, DOOMGENERIC_RESY);
	vulkanEngine->Draw();

	handleKeyInput();
}

void DG_SleepMs(uint32_t ms) {
	SDL_Delay(ms);
}

uint32_t DG_GetTicksMs() {
	return SDL_GetTicks();
}

int DG_GetKey(int* pressed, unsigned char* doomKey) {
	if (s_KeyQueueReadIndex == s_KeyQueueWriteIndex) {
		//key queue is empty
		return 0;
	} else {
		unsigned short keyData = s_KeyQueue[s_KeyQueueReadIndex];
		s_KeyQueueReadIndex++;
		s_KeyQueueReadIndex %= KEYQUEUE_SIZE;

		*pressed = keyData >> 8;
		*doomKey = keyData & 0xFF;

		return 1;
	}

	return 0;
}

void DG_SetWindowTitle(const char* title) {
	vulkanEngine->SetWindowTitle(title);
}
}

int main(int argc, char** argv) {
	doomgeneric_Create(argc, argv);

	for (int i = 0; ; i++) {
		doomgeneric_Tick();
	}


	return 0;
}
