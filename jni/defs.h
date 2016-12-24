#include "glesutil.h"

#define distance(x1,x2,y1,y2) (sqrtf(powf((x2)-(x1),2)+powf((y2)-(y1),2)))
#define xcorrect(x) ((x)-(state->player.base.x+(PLAYER_WIDTH/2.0f)))
#define ycorrect(y) ((y)-(state->player.base.y+(PLAYER_HEIGHT/2.0f)))

// gameplay
#define TID_BACKGROUND 0
#define TID_PLAYER 1
#define TID_CLOUD 2

// ui
#define TID_JOYBASE 0
#define TID_JOYTOP 1

// sounds
#define SID_BACKGROUND 0

#define JOYBASE_SIZE 2.0f
#define JOYTOP_SIZE 1.0f
#define JOYTOP_DIST 1.2f
struct base{
	float x,y,w,h,rot;
};

#define CLOUD_SIZE 2.0f
#define CLOUD_RMDIST 15.0f
struct cloud{
	struct base base;
	struct cloud *next;
};

#define PLAYER_WIDTH 0.658f
#define PLAYER_HEIGHT 0.8f
#define PLAYER_SPEED 0.03f
struct player{
	struct base base;
	float xv,yv;
};

struct state{
	int running;

	int vao,vbo,program;
	struct device device,screen;
	struct pack assets,uiassets;
	struct apack aassets;
	struct{float left,right,bottom,top;}rect;
	struct{ftfont *main;}font;
	struct{int vector,size,texcoords,rot,rgba,projection;}uniform;
	struct crosshair pointer[2];
	
	EGLSurface surface;
	EGLContext context;
	EGLDisplay display;
	struct android_app *app;
	struct jni_info jni_info;

	struct base background,joy_base,joy_top;
	struct player player;
	struct cloud *cloudlist;
};

int process(struct android_app*);
int32_t inputproc(struct android_app*,AInputEvent*);
void cmdproc(struct android_app*,int32_t);
void init_display(struct state*);
void term_display(struct state*);

int core(struct state *state);
void render(struct state *state);
void init(struct state*);
void reset(struct state*);

void draw(struct state*,struct base*);
void uidraw(struct state*,struct base*);
void newcloud(struct state*);
struct cloud *deletecloud(struct state*,struct cloud*,struct cloud*);

