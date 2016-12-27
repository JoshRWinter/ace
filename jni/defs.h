#include "glesutil.h"

#define distance(x1,x2,y1,y2) (sqrtf(powf((x2)-(x1),2)+powf((y2)-(y1),2)))
#define xcorrect(x) ((x)-(state->player.base.x+(PLAYER_WIDTH/2.0f)))
#define ycorrect(y) ((y)-(state->player.base.y+(PLAYER_HEIGHT/2.0f)))

// gameplay
#define TID_BACKGROUND 0
#define TID_PLAYER 1
#define TID_CLOUD 2
#define TID_BULLET 3

// ui
#define TID_JOYBASE 0
#define TID_JOYTOP 1
#define TID_JOYFIRE 2

// sounds
#define SID_BACKGROUND 0

#define JOYBASE_SIZE 2.0f
#define JOYTOP_SIZE 1.0f
#define JOYTOP_DIST 1.2f
#define JOYFIRE_SIZE 1.5f
struct base{
	float x,y,w,h,rot;
};

#define BULLET_WIDTH 0.5f
#define BULLET_HEIGHT 0.35f
struct bullet{
	struct base base;
	float xv,yv;
	struct bullet *next;
};

#define PLAYER_WIDTH 0.658f
#define PLAYER_HEIGHT 0.8f
#define PLAYER_SPEED 0.03f
struct player{
	struct base base;
	float xv,yv;
};

#define CLOUD_SIZE 2.0f
#define CLOUD_RMDIST 15.0f
struct cloud{
	struct base base;
	struct cloud *next;
};

struct state{
	int running;
	int fire;

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

	struct base background,joy_base,joy_top,joy_fire;
	struct player player;
	struct bullet *bulletlist;
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
void newbullet(struct state*,struct base*);
struct bullet *deletebullet(struct state*,struct bullet*,struct bullet*);
void newcloud(struct state*);
struct cloud *deletecloud(struct state*,struct cloud*,struct cloud*);

