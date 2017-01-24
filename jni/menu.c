#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <android_native_app_glue.h>
#include <stdlib.h>
#include "defs.h"

int menu_main(struct state *state){
	struct button buttonplay={{-BUTTON_WIDTH/2.0f,3.0f,BUTTON_WIDTH,BUTTON_HEIGHT,0.0f,1.0f,0.0f},"Play",false};
	struct button buttonaboot={{2.0f,3.0f,BUTTON_WIDTH,BUTTON_HEIGHT,0.0f,1.0f,0.0f},"Aboot",false};
	struct button buttonconf={{-5.0f,3.0f,BUTTON_WIDTH,BUTTON_HEIGHT,0.0f,1.0f,0.0f},"Settings",false};
	struct base title={state->rect.left,state->rect.top,state->rect.right*2.0f,state->rect.bottom*2.0f,0.0f,1.0f,0.0f};
	const char *aboot=
		"~ ACE ~ \n"
		"Programming and Art by Josh Winter\n"
		"yadda yadda";
	while(process(state->app)){
		// draw background
		glUniform4f(state->uniform.rgba,1.0f,1.0f,1.0f,1.0f);
		glBindTexture(GL_TEXTURE_2D,state->uiassets.texture[TID_TITLE].object);
		draw(state,&title);

		// buttons
		if(button_process(state->pointer,&buttonplay)==BUTTON_ACTIVATE){
			return true;
		}
		if(button_process(state->pointer,&buttonaboot)==BUTTON_ACTIVATE){
			if(!menu_message(state,"Aboot",aboot))
				return false;
		}
		if(button_process(state->pointer,&buttonconf)==BUTTON_ACTIVATE){
		}

		// draw buttons
		button_draw(state,&buttonplay);
		button_draw(state,&buttonaboot);
		button_draw(state,&buttonconf);

		eglSwapBuffers(state->display,state->surface);
	}
	return false;
}

int menu_end(struct state *state){
	struct button buttonnew={{0.25f,3.0f,BUTTON_WIDTH,BUTTON_HEIGHT,0.0f,1.0f,0.0f},"Reset",false};
	struct button buttonstop={{-3.0f,3.0f,BUTTON_WIDTH,BUTTON_HEIGHT,0.0f,1.0f,0.0f},"Menu",false};
	char info[26];
	sprintf(info,"SCORE: %d",(int)state->points);
	// show the slow motion death of the player
	state->gamespeed=1.0f;
	int egg=onein(100);
	while(process(state->app)&&state->gamespeed>0.005f){
		state->gamespeed/=1.05f;
		if(!core(state))
			return false;

		render(state);
		eglSwapBuffers(state->display,state->surface);
	}
	if(state->app->destroyRequested)
		return false;
	// make every object slide downwards
	const float vd=0.01f;
	float slide=vd;
	float yoff=state->rect.top*2.0f;
	while(process(state->app)){
		for(struct enemy *enemy=state->enemylist;enemy!=NULL;enemy=enemy->next)
			enemy->base.y+=slide;
		for(struct missile *missile=state->missilelist;missile!=NULL;missile=missile->next)
			missile->base.y+=slide;
		for(struct bullet *bullet=state->bulletlist;bullet!=NULL;bullet=bullet->next)
			bullet->base.y+=slide;
		for(struct smoke *smoke=state->smokelist;smoke!=NULL;smoke=smoke->next)
			smoke->base.y+=slide;
		for(struct health *health=state->healthlist;health!=NULL;health=health->next)
			health->base.y+=slide;
		for(struct cloud *cloud=state->cloudlist;cloud!=NULL;cloud=cloud->next)
			cloud->base.y+=slide;
		for(struct explosion *explosion=state->explosionlist;explosion!=NULL;explosion=explosion->next){
			explosion->cloud.base.y+=slide;
			for(int i=0;i<EXPLOSION_FLASH_COUNT;++i)
				explosion->flash[i].base.y+=slide;
		}

		slide+=vd;

		render(state);

		if(slide>0.5f){
			buttonnew.base.y=yoff+3.0f;
			buttonstop.base.y=yoff+3.0f;

			if(yoff!=0.0f){
				yoff-=(yoff/10.0f);
				if(yoff>-0.005)
					yoff=0.0f;
			}

			// header
			glUniform4f(state->uniform.rgba,1.0f,1.0f,1.0f,1.0f);
			glBindTexture(GL_TEXTURE_2D,state->font.header->atlas);
			drawtextcentered(state->font.header,0.0f,yoff+-4.0f,egg?"lol u ded":"KILLED IN ACTION");

			// end game stats
			glBindTexture(GL_TEXTURE_2D,state->font.button->atlas);
			drawtextcentered(state->font.button,0.0f,yoff-1.0f,info);

			// buttons
			if(button_process(state->pointer,&buttonnew)==BUTTON_ACTIVATE){
				reset(state);
				return true;
			}
			if(button_process(state->pointer,&buttonstop)==BUTTON_ACTIVATE){
				reset(state);
				state->showmenu=true;
				return true;
			}

			// draw buttons
			button_draw(state,&buttonnew);
			button_draw(state,&buttonstop);
		}
		eglSwapBuffers(state->display,state->surface);
	}
	return false;
}

int menu_message(struct state *state,const char *caption,const char *msg){
	struct button buttonok={{-BUTTON_WIDTH/2.0f,3.0f,BUTTON_WIDTH,BUTTON_HEIGHT,0.0f,1.0f,0.0f},"OK",false};
	while(process(state->app)){
		glClear(GL_COLOR_BUFFER_BIT);

		// draw caption
		glUniform4f(state->uniform.rgba,1.0f,1.0f,1.0f,1.0f);
		glBindTexture(GL_TEXTURE_2D,state->font.header->atlas);
		drawtextcentered(state->font.header,0.0f,-3.75f,caption);

		// draw msg
		glBindTexture(GL_TEXTURE_2D,state->font.main->atlas);
		drawtextcentered(state->font.main,0.0f,-2.5f,msg);

		if(button_process(state->pointer,&buttonok)==BUTTON_ACTIVATE){
			return true;
		}

		button_draw(state,&buttonok);

		eglSwapBuffers(state->display,state->surface);
	}
	return false;
}

