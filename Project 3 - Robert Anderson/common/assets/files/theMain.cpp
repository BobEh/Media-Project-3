#include <string>
#include <sstream>
#define BUFFERSIZE 255
#include <stdio.h>
#include <assert.h>
#include <fstream>
#include <iostream>
#include <glad/glad.h>
#include <GLFW\glfw3.h>
#include <fmod.hpp>
#include <map>
#include <vector>
#include <ft2build.h>
#include FT_FREETYPE_H
#define FMOD_TIMEUNIT_MS 0x00000001

static void error_callback(int error, const char* description);
static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);

class FMODWrapper
{
public:
	FMODWrapper();
	bool createSound(bool isStreamed);
	bool playSound(bool isStreamed);
	std::string get_name();
	float getPitch();
	void downPitch(float newPitch);
	void upPitch(float newPitch);
	float getPan();
	void downPan(float newPan);
	void upPan(float newPan);
	float get_volume();
	void turnUpVolume(float volumeToAdd);
	void turnDownVolume(float volumeToSubtract);
	float getPosition();
	float getLength();
	void setPath(std::string newPath);
	bool getPausedStatus();
	void setPausedStatus(bool isPaused);
	std::string getType();
	std::string getFormat();
	std::string get_info();
	std::string getStory();
	void setStory(std::string description);

	FMOD::Sound* _sound;
	FMOD::Channel* _channel;
private:
	FMOD_RESULT _result;
	std::string _path;
	char _name[BUFFERSIZE];
	float _volume;
	float _pitch;
	float _pan;
	unsigned int _position;
	unsigned int _length;
	FMOD_SOUND_TYPE _type;
	FMOD_SOUND_FORMAT _format;
	bool _isPaused;
	bool _isPlaying;
	std::string story;
};

struct point {
	GLfloat x;
	GLfloat y;
	GLfloat s;
	GLfloat t;
};

static unsigned int _vertex_shader, _fragment_shader, _program;
static GLfloat g_currentYPosition = 0.0f;
static GLfloat g_yOffset = 40.0f;
static char g_text_buffer[512];

//Functions
static bool init_gl();
static bool init_text();
static bool init_shaders();
static void render_text(const char* text);
static void errorCheck(FMOD_RESULT);
static void releaseFMOD();
static bool initFMOD();

//GLFW
static int _window_width = 640;
static int _window_height = 480;
static GLFWwindow* _main_window = NULL;

static FT_Library _ft;
static FT_Face _face;
static unsigned int _text_program;
static unsigned int _uniform_tex;
static unsigned int _attribute_coord;
static unsigned int _dp_vbo;
static unsigned int _uniform_color;

static FMOD::System* g_system = 0;
static FMOD::Sound* g_sound = 0;
static FMOD::Channel* g_channel = 0;
static FMOD_RESULT g_result = FMOD_OK;
static int g_releaseCounter = 0;

class cWorld
{
public:

	static cWorld* createWorld();

	FMODWrapper getAudioItem()
	{
		return this->g_m_audioItems.at(this->selectedItem);
	}
	FMODWrapper getAudioItemAtIndex(int index)
	{
		return this->g_m_audioItems.at(index);
	}
	void addAudioItem(int key, FMODWrapper item)
	{
		this->g_m_audioItems.insert({ key, item });
	}
	std::map<int, FMODWrapper> getAllAudioItems()
	{
		return this->g_m_audioItems;
	}

	int selectedItem;

	std::map<int, FMODWrapper> g_m_audioItems;

private:

	static cWorld* pSingletonWorld;
};

cWorld* cWorld::pSingletonWorld = NULL;

cWorld* cWorld::createWorld()
{
	if (cWorld::pSingletonWorld == NULL)
	{
		cWorld::pSingletonWorld = new cWorld();
	}
	pSingletonWorld->selectedItem = 1;
	return cWorld::pSingletonWorld;
}

cWorld* pTheWorld = cWorld::createWorld();

void releaseFMOD() {
	for (int i = 1; i <= pTheWorld->getAllAudioItems().size(); ++i)
	{
		if (pTheWorld->getAudioItemAtIndex(i)._sound)
		{
			g_result = pTheWorld->getAudioItemAtIndex(i)._sound->release();
			errorCheck(g_result);
		}
	}
	if (g_system) {
		g_result = g_system->close();
		errorCheck(g_result);
		g_result = g_system->release();
		errorCheck(g_result);
	}
}

bool initFMOD() {

	g_result = FMOD::System_Create(&g_system);
	errorCheck(g_result);

	g_result = g_system->init(32, FMOD_INIT_NORMAL, 0);
	errorCheck(g_result);

	return true;
}

void errorCheck(FMOD_RESULT result) {

	if (result != FMOD_OK) {
		releaseFMOD();
		system("pause");
		exit(1);
	}
}

bool init_gl() {

	glfwSetErrorCallback(error_callback);

	if (!glfwInit()) {
		fprintf(stderr, "Unable to init glfw\n");
		return false;
	}

	//Full screen
	//_main_window = glfwCreateWindow(1920, 1080, "Media Fundamentals... play sound", glfwGetPrimaryMonitor(), NULL);
	_main_window = glfwCreateWindow(1920, 1080, "Media Fundamentals... play sound", NULL, NULL);

	if (!_main_window)
	{
		glfwTerminate();
		fprintf(stderr, "Unable to create main window glfw\n");
		return false;
	}

	glfwGetWindowSize(_main_window, &_window_width, &_window_height);

	glfwSetKeyCallback(_main_window, key_callback);
	glfwMakeContextCurrent(_main_window);
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
		fprintf(stderr, "Unable to init glad\n");
		return false;
	}
	glfwSwapInterval(1);
	glfwWindowHint(GLFW_SAMPLES, 4);

	// Enable depth test
	glEnable(GL_DEPTH_TEST);
	// Accept fragment if it closer to the camera than the former one
	glDepthFunc(GL_LESS);
	//glEnable(GL_CULL_FACE);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDepthMask(GL_TRUE);

	//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

	glClearColor(0.7f, 0.7f, 0.7f, 1.0f);

	return true;
}

bool init_text() {

	if (FT_Init_FreeType(&_ft)) {
		fprintf(stderr, "Unable to init text...\n");
		return false;
	}
	if (FT_New_Face(_ft, "../common/assets/fonts/arial.ttf", 0, &_face)) {
		fprintf(stderr, "Unable to init text...\n");
		return false;
	}

	//Font size
	FT_Set_Pixel_Sizes(_face, 0, 36);

	if (FT_Load_Char(_face, 'X', FT_LOAD_RENDER))
	{
		fprintf(stderr, "unable to load character\n");
		return false;
	}

	return true;
}

bool init_shaders() {
	//=======================================================================================
//Shaders
	int params = -1;

	glGetProgramiv(_program, GL_LINK_STATUS, &params);
	if (!params) {
		fprintf(stderr, "Unable to link shader program");
		return 1;
	}
	//--	
	FILE* f = fopen("../common/src/shaders/simple_text.vert", "rb");
	long filesize = ftell(f);
	fseek(f, 0, SEEK_END);
	filesize = ftell(f);
	fseek(f, 0, SEEK_SET);
	char* simple_text_vert = (char*)malloc(filesize + 1);
	fread(simple_text_vert, filesize, 1, f);
	fclose(f);
	simple_text_vert[filesize] = 0;
	//--
	f = fopen("../common/src/shaders/simple_text.frag", "rb");
	fseek(f, 0, SEEK_END);
	filesize = ftell(f);
	fseek(f, 0, SEEK_SET);
	char* simple_text_frag = (char*)malloc(filesize + 1);
	fread(simple_text_frag, filesize, 1, f);
	fclose(f);
	simple_text_frag[filesize] = 0;

	//text vertex shader
	unsigned int simple_text_vs = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(simple_text_vs, 1, &simple_text_vert, NULL);
	glCompileShader(simple_text_vs);
	glGetShaderiv(simple_text_vs, GL_COMPILE_STATUS, &params);
	if (!params) {
		fprintf(stderr, "Unable to compile simple text vertex shader");
		return 1;
	}
	unsigned int simple_text_fg = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(simple_text_fg, 1, &simple_text_frag, NULL);
	glCompileShader(simple_text_fg);
	glGetShaderiv(simple_text_fg, GL_COMPILE_STATUS, &params);
	if (!params) {
		fprintf(stderr, "Unable to compile simple text fragment shader");
		return 1;
	}

	_text_program = glCreateProgram();
	glAttachShader(_text_program, simple_text_vs);
	glAttachShader(_text_program, simple_text_fg);
	glLinkProgram(_text_program);

	glGetProgramiv(_text_program, GL_LINK_STATUS, &params);
	if (!params) {
		fprintf(stderr, "Unable to link simple text shader program");
		return 1;
	}

	_uniform_tex = glGetUniformLocation(_text_program, "tex");
	_attribute_coord = glGetAttribLocation(_text_program, "coord");
	_uniform_color = glGetUniformLocation(_text_program, "color");
	glGenBuffers(1, &_dp_vbo);

	glUseProgram(_program);
	return true;
}

void render_text(const char* text) {
	//render_text(const char *text, float x, float y, float sx, float sy)
	float sx = 2.0f / _window_width;
	float sy = 2.0f / _window_height;

	//GLfloat _current_y_position = 30.0f;
	GLfloat xoffset = 8 * sx;
	float x = -1 + xoffset;
	float y = 1 - g_currentYPosition * sy;


	glUseProgram(_text_program);
	GLfloat black[4] = { 0, 0, 0, 1 };
	glUniform4fv(_uniform_color, 1, black);
	const char* p;
	FT_GlyphSlot g = _face->glyph;

	GLuint tex;
	glActiveTexture(GL_TEXTURE0);
	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_2D, tex);
	glUniform1i(_uniform_tex, 0);

	/* We require 1 byte alignment when uploading texture data */
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	/* Clamping to edges is important to prevent artifacts when scaling */
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	/* Linear filtering usually looks best for text */
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	/* Set up the VBO for our vertex data */
	glEnableVertexAttribArray(_attribute_coord);
	glBindBuffer(GL_ARRAY_BUFFER, _dp_vbo);
	glVertexAttribPointer(_attribute_coord, 4, GL_FLOAT, GL_FALSE, 0, 0);


	/* Loop through all characters */
	for (p = text; *p; p++) {
		/* Try to load and render the character */
		if (FT_Load_Char(_face, *p, FT_LOAD_RENDER))
			continue;


		glTexImage2D(
			GL_TEXTURE_2D,
			0,
			GL_RED,
			g->bitmap.width,
			g->bitmap.rows,
			0,
			GL_RED,
			GL_UNSIGNED_BYTE,
			g->bitmap.buffer
		);

		/* Calculate the vertex and texture coordinates */
		float x2 = x + g->bitmap_left * sx;
		float y2 = -y - g->bitmap_top * sy;
		float w = g->bitmap.width * sx;
		float h = g->bitmap.rows * sy;

		point box[4] = {
			{ x2, -y2, 0, 0 },
		{ x2 + w, -y2, 1, 0 },
		{ x2, -y2 - h, 0, 1 },
		{ x2 + w, -y2 - h, 1, 1 },
		};


		/* Draw the character on the screen */
		glBufferData(GL_ARRAY_BUFFER, sizeof(box), box, GL_DYNAMIC_DRAW);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

		/* Advance the cursor to the start of the next character */
		x += (g->advance.x >> 6) * sx;
		y += (g->advance.y >> 6) * sy;
	}
	glDisableVertexAttribArray(_attribute_coord);
	glDeleteTextures(1, &tex);
	glEnableVertexAttribArray(0);

	g_currentYPosition = (g_currentYPosition > 800.0f) ? 30.0f : (g_currentYPosition + g_yOffset);
}

FMODWrapper::FMODWrapper() {
	this->_result = FMOD_OK;
	this->_path = "";
	this->_type = FMOD_SOUND_TYPE_UNKNOWN;
	this->_format = FMOD_SOUND_FORMAT_NONE;
	this->_position = 0.0f;
	this->_length = 0.0f;
	this->_name[BUFFERSIZE - 1] = { '\0' };
	this->_volume = 0.5f;
	this->_pitch = 0.0f;
	this->_pan = 0.0f;
	this->_isPaused = true;
	this->_isPlaying = false;
	this->_sound = 0;
	this->_channel = 0;
}

std::string FMODWrapper::get_name() {
	if (this->_sound) {
		this->_result = this->_sound->getName(_name, BUFFERSIZE);
		errorCheck(_result);
	}
	return _name;
}

float FMODWrapper::getPitch() {
	if (this->_channel) {
		FMODWrapper selectedClip = pTheWorld->getAudioItem();
		this->_result = this->_channel->getPitch(&selectedClip._pitch);
		errorCheck(_result);
	}
	return _pitch;
}

void FMODWrapper::downPitch(float newPitch) {
	if (this->_channel) {
		this->_pitch -= newPitch;
		this->_result = this->_channel->setPitch(_pitch);
		errorCheck(_result);
	}
}

void FMODWrapper::upPitch(float newPitch) {
	if (this->_channel) {
		this->_pitch += newPitch;
		this->_result = this->_channel->setPitch(_pitch);
		errorCheck(_result);
	}
}

float FMODWrapper::getPan() {
	return this->_pan;
}

void FMODWrapper::downPan(float newPan) {
	if (this->_channel) {
		this->_pan -= newPan;
		this->_result = this->_channel->setPan(newPan);
		errorCheck(_result);
	}
}

void FMODWrapper::upPan(float newPan) {
	if (_channel) {
		this->_pan += newPan;
		this->_result = this->_channel->setPan(newPan);
		errorCheck(_result);
	}
}

float FMODWrapper::get_volume() {
	if (this->_channel) {
		this->_result = this->_channel->getVolume(&_volume);
		errorCheck(this->_result);
	}

	return _volume;
}

void FMODWrapper::turnUpVolume(float volumeToAdd) {
	if (this->_channel) {
		this->_volume = this->_volume + volumeToAdd;
		this->_result = this->_channel->setVolume(_volume);
		errorCheck(_result);
	}
}

void FMODWrapper::turnDownVolume(float volumeToSubtract) {
	if (this->_channel) {
		this->_volume = this->_volume - volumeToSubtract;
		this->_result = this->_channel->setVolume(_volume);
		errorCheck(_result);
	}
}

float FMODWrapper::getPosition() {
	if (this->_channel)
	{
		this->_result = this->_channel->getPosition(&_position, FMOD_TIMEUNIT_MS);
		errorCheck(_result);
	}
	return _position;
}

float FMODWrapper::getLength() {
	if (this->_sound)
	{
		this->_sound->getLength(&_length, FMOD_TIMEUNIT_MS);
		errorCheck(_result);
	}
	return _length;
}

void FMODWrapper::setPath(std::string newPath) {
	this->_path = newPath;
}

bool FMODWrapper::getPausedStatus() {
	if (this->_channel) {
		this->_result = this->_channel->getPaused(&_isPaused);
		errorCheck(_result);
	}
	return _isPaused;
}
void FMODWrapper::setPausedStatus(bool isPaused) {
	if (this->_channel) {
		_isPaused = isPaused;
		_result = _channel->setPaused(_isPaused);
	}
}

std::string FMODWrapper::getStory()
{
	return this->story;
}

void FMODWrapper::setStory(std::string description)
{
	this->story = description;
}

std::string FMODWrapper::getType() {
	if (_sound) {
		_result = _sound->getFormat(&this->_type, 0, 0, 0);
		errorCheck(_result);
	}
	std::string value = "";
	if (_type == FMOD_SOUND_TYPE_WAV)
	{
		value = "FMOD_SOUND_TYPE_WAV";
	}
	else if (_type == FMOD_SOUND_TYPE_MPEG)
	{
		value = "FMOD_SOUND_TYPE_MPEG";
	}
	else if (_type == FMOD_SOUND_TYPE_OGGVORBIS)
	{
		value = "FMOD_SOUND_TYPE_OGGVORBIS";
	}
	else if (_type == FMOD_SOUND_TYPE_AIFF)
	{
		value = "FMOD_SOUND_TYPE_AIFF";
	}
	else if (_type == FMOD_SOUND_TYPE_ASF)
	{
		value = "FMOD_SOUND_TYPE_ASF";
	}
	else if (_type == FMOD_SOUND_TYPE_DLS)
	{
		value = "FMOD_SOUND_TYPE_DLS";
	}
	else if (_type == FMOD_SOUND_TYPE_FLAC)
	{
		value = "FMOD_SOUND_TYPE_FLAC";
	}
	else if (_type == FMOD_SOUND_TYPE_FSB)
	{
		value = "FMOD_SOUND_TYPE_FSB";
	}
	else if (_type == FMOD_SOUND_TYPE_IT)
	{
		value = "FMOD_SOUND_TYPE_IT";
	}
	else if (_type == FMOD_SOUND_TYPE_MIDI)
	{
		value = "FMOD_SOUND_TYPE_MIDI";
	}
	else if (_type == FMOD_SOUND_TYPE_MOD)
	{
		value = "FMOD_SOUND_TYPE_MOD";
	}
	else if (_type == FMOD_SOUND_TYPE_PLAYLIST)
	{
		value = "FMOD_SOUND_TYPE_PLAYLIST";
	}
	else if (_type == FMOD_SOUND_TYPE_RAW)
	{
		value = "FMOD_SOUND_TYPE_RAW";
	}
	else if (_type == FMOD_SOUND_TYPE_S3M)
	{
		value = "FMOD_SOUND_TYPE_S3M";
	}
	else if (_type == FMOD_SOUND_TYPE_USER)
	{
		value = "FMOD_SOUND_TYPE_USER";
	}
	else if (_type == FMOD_SOUND_TYPE_XM)
	{
		value = "FMOD_SOUND_TYPE_XM";
	}
	else if (_type == FMOD_SOUND_TYPE_XMA)
	{
		value = "FMOD_SOUND_TYPE_XMA";
	}
	else if (_type == FMOD_SOUND_TYPE_AUDIOQUEUE)
	{
		value = "FMOD_SOUND_TYPE_AUDIOQUEUE";
	}
	else if (_type == FMOD_SOUND_TYPE_AT9)
	{
		value = "FMOD_SOUND_TYPE_AT9";
	}
	else if (_type == FMOD_SOUND_TYPE_VORBIS)
	{
		value = "FMOD_SOUND_TYPE_VORBIS";
	}
	else if (_type == FMOD_SOUND_TYPE_MEDIA_FOUNDATION)
	{
		value = "FMOD_SOUND_TYPE_MEDIA_FOUNDATION";
	}
	else if (_type == FMOD_SOUND_TYPE_MEDIACODEC)
	{
		value = "FMOD_SOUND_TYPE_MEDIACODEC";
	}
	else if (_type == FMOD_SOUND_TYPE_FADPCM)
	{
		value = "FMOD_SOUND_TYPE_FADPCM";
	}
	else if (_type == FMOD_SOUND_TYPE_MAX)
	{
		value = "FMOD_SOUND_TYPE_MAX";
	}
	return value;
}

std::string FMODWrapper::getFormat() {
	if (_sound) {
		_result = _sound->getFormat(0, &this->_format, 0, 0);
	}
	std::string value = "";
	if (_format == FMOD_SOUND_FORMAT_PCM16)
	{
		value = "FMOD_SOUND_FORMAT_PCM16";
	}
	else if (_format == FMOD_SOUND_FORMAT_PCM8)
	{
		value = "FMOD_SOUND_FORMAT_PCM8";
	}
	else if (_format == FMOD_SOUND_FORMAT_PCM24)
	{
		value = "FMOD_SOUND_FORMAT_PCM24";
	}
	else if (_format == FMOD_SOUND_FORMAT_PCM32)
	{
		value = "FMOD_SOUND_FORMAT_PCM32";
	}
	else if (_format == FMOD_SOUND_FORMAT_PCMFLOAT)
	{
		value = "FMOD_SOUND_FORMAT_PCMFLOAT";
	}
	else if (_format == FMOD_SOUND_FORMAT_BITSTREAM)
	{
		value = "FMOD_SOUND_FORMAT_BITSTREAM";
	}
	else if (_format == FMOD_SOUND_FORMAT_MAX)
	{
		value = "FMOD_SOUND_FORMAT_MAX";
	}
	return value;
}

//create get info method
std::string FMODWrapper::get_info() {
	std::ostringstream ss;

	//get name
	ss << "Name: " << this->get_name() << "    ";
	ss << "Volume: " << this->get_volume() << "    ";
	ss << "Is paused: " << ((this->getPausedStatus()) ? "TRUE" : "FALSE");

	std::string result = ss.str();
	return result;
}

//User Inputs
static void error_callback(int error, const char* description)
{
	fprintf(stderr, "Error: %s\n", description);
}

static bool shiftKeyPressed(int mods)
{
	if (mods == GLFW_MOD_SHIFT)
	{
		return true;
	}
	return false;
}

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {

	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
		glfwSetWindowShouldClose(window, GLFW_TRUE);
	}
	if (key == GLFW_KEY_SPACE && action == GLFW_PRESS) {
		if (pTheWorld->getAudioItem().getPausedStatus() == true)
		{
			pTheWorld->getAudioItem().setPausedStatus(false);
		}
		else
		{
			pTheWorld->getAudioItem().setPausedStatus(true);
		}
	}
	if (key == GLFW_KEY_1 && action == GLFW_PRESS)
	{
		pTheWorld->selectedItem = 1;
	}
	if (key == GLFW_KEY_2 && action == GLFW_PRESS)
	{
		pTheWorld->selectedItem = 2;
	}
	if (key == GLFW_KEY_3 && action == GLFW_PRESS)
	{
		pTheWorld->selectedItem = 3;
	}
	if (key == GLFW_KEY_4 && action == GLFW_PRESS)
	{
		pTheWorld->selectedItem = 4;
	}
	if (key == GLFW_KEY_5 && action == GLFW_PRESS)
	{
		pTheWorld->selectedItem = 5;
	}
	if (key == GLFW_KEY_6 && action == GLFW_PRESS)
	{
		pTheWorld->selectedItem = 6;
	}
	if (key == GLFW_KEY_7 && action == GLFW_PRESS)
	{
		pTheWorld->selectedItem = 7;
	}
	if (key == GLFW_KEY_8 && action == GLFW_PRESS)
	{
		pTheWorld->selectedItem = 8;
	}
	if (key == GLFW_KEY_9 && action == GLFW_PRESS)
	{
		pTheWorld->selectedItem = 9;
	}
	if (key == GLFW_KEY_A && action == GLFW_PRESS)
	{
		pTheWorld->selectedItem = 10;
	}
	if (key == GLFW_KEY_B && action == GLFW_PRESS)
	{
		pTheWorld->selectedItem = 11;
	}
	if (key == GLFW_KEY_C && action == GLFW_PRESS)
	{
		pTheWorld->selectedItem = 12;
	}
	if (key == GLFW_KEY_D && action == GLFW_PRESS)
	{
		pTheWorld->selectedItem = 13;
	}
	if (key == GLFW_KEY_E && action == GLFW_PRESS)
	{
		pTheWorld->selectedItem = 14;
	}
	if (key == GLFW_KEY_UP && action == GLFW_PRESS)
	{
		pTheWorld->getAudioItem().turnUpVolume(0.1f);
	}
	if (key == GLFW_KEY_DOWN && action == GLFW_PRESS)
	{
		pTheWorld->getAudioItem().turnDownVolume(0.1f);
	}
	if (key == GLFW_KEY_LEFT && action == GLFW_PRESS)
	{
		pTheWorld->getAudioItem().downPan(0.1f);
	}
	if (key == GLFW_KEY_RIGHT && action == GLFW_PRESS)
	{
		pTheWorld->getAudioItem().upPan(0.1f);
	}
	if (key == GLFW_KEY_W && action == GLFW_PRESS)
	{
		pTheWorld->getAudioItem().upPitch(0.1f);
	}
	if (key == GLFW_KEY_S && action == GLFW_PRESS)
	{
		pTheWorld->getAudioItem().downPitch(0.1f);
	}
}

bool FMODWrapper::createSound(bool streamed) {
	if (g_system) {
		this->_result = g_system->createSound(_path.c_str(), (streamed) ? FMOD_CREATESTREAM : FMOD_DEFAULT, 0, &_sound);
		errorCheck(_result);
		return true;
	}

	return false;
}

bool FMODWrapper::playSound(bool streamed)
{
	if (g_system) {
		this->_result = g_system->playSound(_sound, 0, _isPaused, &_channel);
		errorCheck(_result);
		return true;
	}

	return false;
}

int main() {

	bool validInput = false;
	std::ifstream soundLibrary;
	while (!validInput)
	{
		std::string userInput;
		std::cout << "Please choose to use either compressed (c) or uncompressed (u) files to run the program: ";
		std::cin >> userInput;

		if (userInput == "u" || userInput == "U")
		{
			soundLibrary.open("soundLibrary.txt");
			validInput = true;
		}
		else if (userInput == "c" || userInput == "C")
		{
			soundLibrary.open("soundLibraryCompressed.txt");
			validInput = true;
		}
		else
		{
			std::cout << "\nInvalid choice. Please enter a 'u' or a 'c'.\n";
		}
	}
	if (!soundLibrary)
	{
		std::cout << "Unable to open text file. Might be in the wrong location?" << std::endl;
		system("pause");
	}

	fprintf(stdout, "Init opengl ...\n");
	assert(init_gl());

	fprintf(stdout, "Init text...\n");
	assert(init_text());

	fprintf(stdout, "Init shaders...\n");
	assert(init_shaders());

	fprintf(stdout, "Ready ...!\n");

	//Init FMOD here not inside the loop
	initFMOD();

	//Make stories vec
	std::vector<std::string> storiesVec;
	storiesVec.push_back("There is a house party going on.");
	storiesVec.push_back("A man leaves the party.");
	storiesVec.push_back("The man walks to his car.");
	storiesVec.push_back("The man gets in his car and pulls away.");
	storiesVec.push_back("The man is driving home.");
	storiesVec.push_back("The police pull the man over.");
	storiesVec.push_back("The man is frustrated.");
	storiesVec.push_back("The cop approaches the vehicle.");
	storiesVec.push_back("The cop knocks on the window.");
	storiesVec.push_back("The man rolls down his window.");
	storiesVec.push_back("The cop asks the man a few questions.");
	storiesVec.push_back("The man answers his questions.");
	storiesVec.push_back("The man rolls up his window.");
	storiesVec.push_back("The man drives away after a warning from the cop.");

	//Create and load audio items
	std::string line;
	FMODWrapper item;
	int key = 1;
	while (std::getline(soundLibrary, line))
	{
		if (line.length() > 0)
		{
			item.setPath(line.c_str());
			item.createSound(true);
			item.playSound(true);
			pTheWorld->addAudioItem( key, item );
		}
		key += 1;
	}

	//Add stories
	int index = 0;
	for (std::map<int, FMODWrapper>::iterator itMap = pTheWorld->g_m_audioItems.begin(); itMap != pTheWorld->g_m_audioItems.end(); ++itMap, ++index)
	{
		itMap->second.setStory(storiesVec.at(index));
	}

	//Main loop
	while (!glfwWindowShouldClose(_main_window)) {

		//Reset text y position
		g_currentYPosition = 30.0f;

		// Clear the screen
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glUseProgram(_program);

		bool is_paused = false;
		unsigned int pos;

		FMOD::Sound* current_sound = 0;


		if (g_channel) {
			//get is paused
			g_result = g_channel->getPaused(&is_paused);
			errorCheck(g_result);
			g_result = g_channel->getPosition(&pos, FMOD_TIMEUNIT_MS);
			errorCheck(g_result);
			g_result = g_channel->getCurrentSound(&current_sound);
			errorCheck(g_result);
		}

		const int buffer_size = 255;
		char name[buffer_size] = { '\0' };
		unsigned int len;

		if (g_sound) {
			g_result = g_sound->getName(name, buffer_size);
			errorCheck(g_result);
			g_result = g_sound->getLength(&len, FMOD_TIMEUNIT_MS);
			errorCheck(g_result);
		}

		render_text("=====================================================");
		render_text("Media Fundamentals play sound...");
		render_text("=====================================================");
		render_text("Press ESC to Exit!");
		render_text("Press SPACE to pause and play.");
		render_text("Press the following keys in order to go to that audio clip:");
		render_text("1 - 9 (for first clips)");
		render_text("A - E (for last clips)");
		render_text("Press UP or DOWN to control the volume.");
		render_text("Press LEFT and RIGHT to control the pan.");
		render_text("=====================================================");
		render_text("The story so far:");
		std::string theStory = pTheWorld->getAudioItem().getStory();
		render_text(theStory.c_str());
		//print numbers, strings using buffer
		std::string fileInfo = pTheWorld->getAudioItem().get_info();
		render_text(fileInfo.c_str());
		sprintf(g_text_buffer, "Current volume: %f", pTheWorld->getAudioItem().get_volume());
		render_text(g_text_buffer);
		std::string fileType = pTheWorld->getAudioItem().getType();
		render_text("File type: ");
		render_text(fileType.c_str());
		std::string fileFormat = pTheWorld->getAudioItem().getFormat();
		render_text("File format: ");
		render_text(fileFormat.c_str());
		sprintf(g_text_buffer, "Current pan: %f", pTheWorld->getAudioItem().getPan());
		render_text(g_text_buffer);
		sprintf(g_text_buffer, "Current position: %f", pTheWorld->getAudioItem().getPosition());
		render_text(g_text_buffer);
		sprintf(g_text_buffer, "File length: %f", pTheWorld->getAudioItem().getLength());
		render_text(g_text_buffer);
		sprintf(g_text_buffer, "Current pitch: %f", pTheWorld->getAudioItem().getPitch());
		render_text(g_text_buffer);
		render_text("=====================================================");



		glfwSwapBuffers(_main_window);
		glfwPollEvents();

		glfwGetWindowSize(_main_window, &_window_width, &_window_height);

	}

	glfwDestroyWindow(_main_window);
	glfwTerminate();

	releaseFMOD();

	return 0;
}