#ifndef _SELECTOR_H_
#define _SELECTOR_H_

#include <stdio.h>
#include <string>
#include <unistd.h>
#include <pthread.h>
#include <SDL.h>
#include <SDL_ttf.h>

// static TTF_Font *gFont = NULL;

class CSelector
{
protected:
	int 		m_window_w;
	int 		m_window_h;
	uint8_t		m_r;
	uint8_t 	m_g;
	uint8_t 	m_b;
	uint8_t 	m_a;
	int	        m_x;
	int			m_y;
	int			m_w;
	int			m_h;

	bool 		m_mouse_motion_on;
	bool		m_mouse_lt_keydown;	// left top
	bool		m_mouse_rb_keydown; // right bottom
	bool		m_mouse_keydown;

	std::string m_title;
	pthread_mutex_t	m_mutex;

	static TTF_Font* 	m_Font;
	SDL_Texture* 		m_titlefont_texture;
	SDL_Texture*		m_wh_texture;

	int 				m_FontWidth;
	int 				m_FontHeight;

public:
	CSelector(int window_w, int window_h);

	~CSelector();

	void SetColor(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 0x80);

	void SetGeometryInfo(uint32_t x, uint32_t y, uint32_t w, uint32_t h);

	void GetGeometryInfo(uint32_t* x, uint32_t* y, uint32_t* w, uint32_t* h);

	void SetTitle(SDL_Renderer* r, const char* title);

	std::string GetTitle(std::string& title);

	void Draw(SDL_Renderer* renderer);

	void ProcessMouseEvent(SDL_Event *e);
	
protected:

	static bool InitFont();

	void FreeTitleTexture();

	void GenTitleTexture(SDL_Renderer* r);

	void FreeWHTexture();

	void GenWHTexture(SDL_Renderer* r);
};
#endif