#include "selector.h"

TTF_Font* CSelector::m_Font = NULL;

CSelector::CSelector(int window_w, int window_h): 
m_window_w(window_w), m_window_h(window_h), m_r(0), m_g(0), m_b(0), m_a(0x80), m_x(0), m_y(0), m_w(0), m_h(0)
{
	m_titlefont_texture = m_wh_texture = NULL;
	m_mouse_motion_on = m_mouse_lt_keydown = m_mouse_rb_keydown = m_mouse_keydown = false;

	pthread_mutex_init(&m_mutex, NULL);

	InitFont();
}

CSelector::~CSelector()
{
	if (m_Font)
	{
		TTF_CloseFont(m_Font);
		m_Font = NULL;
	}

	FreeTitleTexture();
	FreeWHTexture();

	TTF_Quit();

	pthread_mutex_destroy(&m_mutex);
}

void CSelector::SetColor(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
	m_r = r;
	m_g = g;
	m_b = b;
	m_b = a;
}

void CSelector::SetGeometryInfo(uint32_t x, uint32_t y, uint32_t w, uint32_t h)
{
	m_x = x;
	m_y = y;
	m_w = w;
	m_h = h;
}

void CSelector::GetGeometryInfo(uint32_t* x, uint32_t* y, uint32_t* w, uint32_t* h)
{
	*x = m_x;
	*y = m_y;
	*w = m_w;
	*h = m_h;
}

void CSelector::SetTitle(SDL_Renderer* r, const char* title)
{
	m_title = title;
	GenTitleTexture(r);
}

std::string CSelector::GetTitle(std::string& title)
{
	title = m_title;
	return m_title;
}

void CSelector::Draw(SDL_Renderer* renderer)
{
	SDL_Rect rect;

	pthread_mutex_lock(&m_mutex);
	
	if (m_mouse_motion_on == true)
	{
		if (m_mouse_lt_keydown == true)
		{
			rect.x = m_x - 4;
			rect.y = m_y - 4;
			rect.w = 8;
			rect.h = 8;

			SDL_SetRenderDrawColor(renderer, 0xFF, 0xFF, m_b, 0xFF);
			SDL_RenderFillRect(renderer, &rect);
		}
		else if (m_mouse_rb_keydown == true)
		{
			rect.x = m_x + m_w - 4;
			rect.y = m_y + m_h - 4;
			rect.w = 8;
			rect.h = 8;

			SDL_SetRenderDrawColor(renderer, 0xFF, 0xFF, m_b, 0xFF);
			SDL_RenderFillRect(renderer, &rect);
		}
		else if (m_mouse_keydown == true)
		{
			rect.x = m_x - 4;
			rect.y = m_y - 4;
			rect.w = 8;
			rect.h = 8;

			SDL_SetRenderDrawColor(renderer, 0xFF, 0xFF, m_b, 0xFF);
			SDL_RenderFillRect(renderer, &rect);

			rect.x = m_x + m_w - 4;
			rect.y = m_y + m_h - 4;
			rect.w = 8;
			rect.h = 8;

			SDL_SetRenderDrawColor(renderer, 0xFF, 0xFF, m_b, 0xFF);
			SDL_RenderFillRect(renderer, &rect);			
		}
		else
		{
			rect.x = m_x - 4;
			rect.y = m_y - 4;
			rect.w = 8;
			rect.h = 8;

			SDL_SetRenderDrawColor(renderer, 0xFF, 0xFF, m_b, 0xFF);
			SDL_RenderDrawRect(renderer, &rect);

			rect.x = m_x + m_w - 4;
			rect.y = m_y + m_h - 4;
			rect.w = 8;
			rect.h = 8;

			SDL_SetRenderDrawColor(renderer, 0xFF, 0xFF, m_b, 0xFF);
			SDL_RenderDrawRect(renderer, &rect);
		}
	}

	// Title
	GenTitleTexture(renderer);

	if (m_titlefont_texture)
	{
		rect.x = m_x;
		rect.y = m_y;
		rect.w = m_FontWidth;
		rect.h = m_FontHeight;
		SDL_RenderCopy(renderer, m_titlefont_texture, NULL, &rect);
	}

	// Width & Height
	GenWHTexture(renderer);
	
	if (m_wh_texture)
	{
		rect.x = m_x;
		rect.y = m_y + m_FontHeight;
		rect.w = m_FontWidth;
		rect.h = m_FontHeight;
		SDL_RenderCopy(renderer, m_wh_texture, NULL, &rect);
	}

	rect.x = m_x;
	rect.y = m_y;
	rect.w = m_w;
	rect.h = m_h;

	SDL_SetRenderDrawColor(renderer, m_r, m_g, m_b, m_a);
	SDL_RenderFillRect(renderer, &rect);

	pthread_mutex_unlock(&m_mutex);
}

void CSelector::ProcessMouseEvent(SDL_Event *e)
{
	int x, y;

	pthread_mutex_lock(&m_mutex);

	if (e)
	{
		if (e->type == SDL_MOUSEMOTION)
		{
			x = e->motion.x;
			y = e->motion.y;

			if (m_mouse_lt_keydown == false && m_mouse_rb_keydown == false && m_mouse_keydown == false)
			{
				if ((x >= m_x && x <= m_x + m_w) && (y >= m_y && y <= m_y + m_h))
				{
					if (m_mouse_motion_on == false)
						m_mouse_motion_on = true;
				}
				else
					m_mouse_motion_on = false;
				
			}
			else
			{
				if (m_mouse_lt_keydown == true)
				{
					if ((x < m_x + m_w && m_x + m_w - x > 10) && (y < m_y + m_h && m_y + m_h - y > 10))
					{
						m_w = m_w - (x - m_x);
						m_h = m_h - (y - m_y);
						m_x = x;
						m_y = y;
					}
				}
				else if (m_mouse_rb_keydown == true)
				{
					if ((x > m_x && x - m_x > 10) && (y > m_y && y - m_y > 10))
					{
						m_w = x - m_x;
						m_h = y - m_y;
					}
				}
				else if (m_mouse_keydown == true)
				{
					if ((x - (m_w >> 1)) >= 0)
						m_x = x - (m_w >> 1);

					if ((m_x + m_w) > m_window_w)
						m_x = (m_window_w - m_w);

					if ((y - (m_h >> 1)) >= 0)
						m_y = y - (m_h >> 1);

					if ((m_y + m_h) > m_window_h)
						m_y = (m_window_h - m_h);
				}
			}
		}

		if (e->type == SDL_MOUSEBUTTONDOWN && e->button.button == SDL_BUTTON_LEFT)
		{
			if (m_mouse_motion_on == true)
			{
				x = e->button.x;
				y = e->button.y;
				
				if ((x - m_x <= 6) && (y - m_y <= 6))
				{
					if (m_mouse_lt_keydown == false)
						m_mouse_lt_keydown = true;
				}
				else if ((m_x + m_w - x <= 6) && (m_y + m_h -y <= 6))
				{
					if (m_mouse_rb_keydown == false)
						m_mouse_rb_keydown = true;
				}
				else 
				{
					if (m_mouse_keydown == false)
						m_mouse_keydown = true;
				}
			}
		}
		
		if (e->type == SDL_MOUSEBUTTONUP && e->button.button == SDL_BUTTON_LEFT)
		{
			if (m_mouse_lt_keydown == true)
				m_mouse_lt_keydown = false;

			if (m_mouse_rb_keydown == true)
				m_mouse_rb_keydown = false;

			if (m_mouse_keydown == true)
				m_mouse_keydown = false;
		}
	}

	pthread_mutex_unlock(&m_mutex);
}

// --------------------------------------------------------------------------------------
// Protected Member Function
// --------------------------------------------------------------------------------------
bool CSelector::InitFont()
{
	bool ret = true;

	if (m_Font == NULL)
	{
		if (TTF_Init() == -1)
		{
			printf( "SDL_ttf could not initialize! SDL_ttf Error: %s\n", TTF_GetError() );
			ret = false;
		}
		else
		{
			m_Font = TTF_OpenFont("/usr/share/fonts/truetype/droid/DroidSans.ttf", 16);

			if (m_Font == NULL)
			{
				printf( "Failed to load lazy font! SDL_ttf Error: %s\n", TTF_GetError() );
				ret = false;
			}
		}
	}

	return ret;
}

void CSelector::FreeTitleTexture()
{
	if (m_titlefont_texture)
	{
		SDL_DestroyTexture(m_titlefont_texture);
		m_titlefont_texture = NULL;
	}
}

void CSelector::GenTitleTexture(SDL_Renderer* r)
{
	SDL_Surface* surface = NULL;
	SDL_Color color = {m_r, m_g, m_b};
	char title[256] = {0};

	FreeTitleTexture();

	snprintf(title, 256, "%s(%d,%d)", m_title.c_str(), m_x, m_y);
	surface = TTF_RenderText_Solid(m_Font, title, color);
	if (surface == NULL)
	{
		printf( "Unable to render text surface! SDL_ttf Error: %s\n", TTF_GetError() );
	}
	else
	{
		m_titlefont_texture 	= SDL_CreateTextureFromSurface(r, surface);
		m_FontWidth 	= surface->w;
		m_FontHeight 	= surface->h;

		if (m_titlefont_texture == NULL)
		{
			printf( "Unable to create texture from rendered text! SDL Error: %s\n", SDL_GetError() );
		}

		SDL_FreeSurface(surface);
	}
}

void CSelector::FreeWHTexture()
{
	if (m_wh_texture)
	{
		SDL_DestroyTexture(m_wh_texture);
		m_wh_texture = NULL;
	}
}

void CSelector::GenWHTexture(SDL_Renderer* r)
{
	SDL_Surface* surface = NULL;
	SDL_Color color = {m_r, m_g, m_b};
	char wh[256] = {0};

	FreeWHTexture();

	snprintf(wh, 256, "(%d,%d)", m_w, m_h);
	surface = TTF_RenderText_Solid(m_Font, wh, color);
	if (surface == NULL)
	{
		printf( "Unable to render text surface! SDL_ttf Error: %s\n", TTF_GetError() );
	}
	else
	{
		m_wh_texture 	= SDL_CreateTextureFromSurface(r, surface);
		m_FontWidth 	= surface->w;
		m_FontHeight 	= surface->h;

		if (m_wh_texture == NULL)
		{
			printf( "Unable to create texture from rendered text! SDL Error: %s\n", SDL_GetError() );
		}

		SDL_FreeSurface(surface);
	}
}