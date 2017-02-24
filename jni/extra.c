#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <android_native_app_glue.h>
#include <stdlib.h>
#include <stdio.h>
#include "defs.h"

int get_pilot_skill(struct state *state){
	const float weight[]={0.3f,0.25f,0.25f,0.1f,0.1f};
	int skill=0;

	// weighted average of high scores / 15
	for(int i=0;i<HIGHSCORE_COUNT;++i)
		skill+=state->highscore[i]*weight[HIGHSCORE_COUNT-1-i];
	skill/=15;

	// plus DFCs * 3
	skill += state->stat.dfc*3;

	// plus air medals
	skill += state->stat.am;

	return skill;
}

char *get_pilot_class(struct state *state){
	int skill=get_pilot_skill(state);
	if(skill<70)
		return "Novice";
	else if(skill < 120)
		return "Intermediate";
	else if(skill < 160)
		return "Professional";
	else
		return "Ace";
}

void load_settings(struct state *state){
	FILE *file=fopen(DATAPATH"/00","rb");

	if(!file){
		state->vibrate=true;
		state->sounds=true;
		state->music=true;
		return;
	}

	fread(&state->vibrate,sizeof(int),1,file);
	fread(&state->sounds,sizeof(int),1,file);
	fread(&state->music,sizeof(int),1,file);

	fclose(file);
}

void save_settings(struct state *state){
	FILE *file=fopen(DATAPATH"/00","wb");

	if(!file){
		logcat("could not write to settings file for some reason");
		return;
	}

	fwrite(&state->vibrate,sizeof(int),1,file);
	fwrite(&state->sounds,sizeof(int),1,file);
	fwrite(&state->music,sizeof(int),1,file);

	fclose(file);
}

void load_stats(struct state *state){
	FILE *file=fopen(DATAPATH"/01","rb");

	if(!file){
		memset(state->highscore,0,sizeof(int)*HIGHSCORE_COUNT);
		memset(&state->stat,0,sizeof(struct stat));
		return;
	}

	fread(state->highscore,sizeof(int),HIGHSCORE_COUNT,file);
	fread(&state->stat.dfc,sizeof(int),1,file);
	fread(&state->stat.am,sizeof(int),1,file);
	fclose(file);
}

void save_stats(struct state *state){
	FILE *file=fopen(DATAPATH"/01","wb");

	if(!file){
		logcat("could not save highscores for some reason");
		return;
	}

	fwrite(state->highscore,sizeof(int),HIGHSCORE_COUNT,file);
	fwrite(&state->stat.dfc,sizeof(int),1,file);
	fwrite(&state->stat.am,sizeof(int),1,file);
	fclose(file);
}

static void swap(int *i,int *j){
	int temp=*i;
	*i=*j;
	*j=temp;
}
void selection(int *a){
	const int n=5;
	int	i,j;
	for (j = 0; j < n-1; j++) {
		int iMin = j;
		for (i = j+1; i < n; i++) {
			if (a[i] < a[iMin]) {
				iMin = i;
			}
		}
		if(iMin != j) {
			swap(a+j, a+iMin);
		}
	}
}

int button_process(struct crosshair *p,struct button *b){
	if(p[0].x>b->base.x&&p[0].x<b->base.x+BUTTON_WIDTH&&p[0].y>b->base.y&&p[0].y<b->base.y+BUTTON_HEIGHT){
		if(p[0].active){
			b->active=true;
			return BUTTON_PRESS;
		}
		else if(b->active){
			b->active=false;
			return BUTTON_ACTIVATE;
		}
	}
	b->active=false;
	return 0;
}

void button_draw(struct state *state,struct button *button){
	float yoff;
	struct button b=*button;
	if(b.active){
		yoff=0.075f;
		b.base.y+=yoff;
	}
	else
		yoff=0.0f;

	glUniform4f(state->uniform.rgba,1.0f,1.0f,1.0f,1.0f);
	glBindTexture(GL_TEXTURE_2D,state->uiassets.texture[TID_BUTTON].object);
	uidraw(state,&b.base);

	glUniform4f(state->uniform.rgba,0.0f,0.0f,0.0f,1.0f);
	glBindTexture(GL_TEXTURE_2D,state->font.button->atlas);
	drawtextcentered(state->font.button,b.base.x+(BUTTON_WIDTH/2.0f),b.base.y+(BUTTON_HEIGHT/2.0f)-(state->font.button->fontsize/2.0f),b.label);
}

int collide(struct base *a,struct base *b,float tolerance){
	return a->x+a->w>b->x+tolerance&&a->x<b->x+(b->w-tolerance)&&a->y+a->h>b->y+tolerance&&a->y<b->y+(b->h-tolerance);
}

void draw(struct state *state,struct base *base){
	// texcoords
	float size=1.0f/base->count;
	float left=size*base->frame;
	float right=left+size;

	float x=base->x;
	float y=base->y;
	glUniform4f(state->uniform.texcoords,left,right,0.0f,1.0f);
	glUniform2f(state->uniform.vector,xcorrect(x),ycorrect(y));
	glUniform2f(state->uniform.size,base->w,base->h);
	glUniform1f(state->uniform.rot,base->rot);

	glDrawArrays(GL_TRIANGLE_STRIP,0,4);
}

void uidraw(struct state *state,struct base *base){
	glUniform4f(state->uniform.texcoords,0.0f,1.0f,0.0f,1.0f);
	glUniform2f(state->uniform.vector,base->x,base->y);
	glUniform2f(state->uniform.size,base->w,base->h);
	glUniform1f(state->uniform.rot,base->rot);

	glDrawArrays(GL_TRIANGLE_STRIP,0,4);
}

