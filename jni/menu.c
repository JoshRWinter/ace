#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <android_native_app_glue.h>
#include <stdlib.h>
#include "defs.h"

int menu_main(struct state *state){
	struct button buttonplay={{-BUTTON_WIDTH/2.0f,3.0f,BUTTON_WIDTH,BUTTON_HEIGHT,0.0f,1.0f,0.0f},"Play",false};
	struct button buttonaboot={{2.0f,3.0f,BUTTON_WIDTH,BUTTON_HEIGHT,0.0f,1.0f,0.0f},"Aboot",false};
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

		// draw buttons
		button_draw(state,&buttonplay);
		button_draw(state,&buttonaboot);

		eglSwapBuffers(state->display,state->surface);
	}
	return false;
}

int menu_end(struct state *state){
	struct button buttonnew={{1.0f,3.0f,BUTTON_WIDTH,BUTTON_HEIGHT,0.0f,1.0f,0.0f},"Reset",false};
	struct button buttonstop={{-2.0f,3.0f,BUTTON_WIDTH,BUTTON_HEIGHT,0.0f,1.0f,0.0f},"Menu",false};
	char info[100];
	sprintf(info,"You scored %d points!",(int)state->points);
	reset(state);
	while(process(state->app)){
		glClear(GL_COLOR_BUFFER_BIT);

		// header
		glUniform4f(state->uniform.rgba,1.0f,1.0f,1.0f,1.0f);
		glBindTexture(GL_TEXTURE_2D,state->font.header->atlas);
		drawtextcentered(state->font.header,0.0f,-3.0f,"Game Over");

		// end game stats
		glBindTexture(GL_TEXTURE_2D,state->font.main->atlas);
		drawtextcentered(state->font.main,0.0f,0.0f,info);

		// buttons
		if(button_process(state->pointer,&buttonnew)==BUTTON_ACTIVATE){
			return true;
		}
		if(button_process(state->pointer,&buttonstop)==BUTTON_ACTIVATE){
			state->showmenu=true;
			return true;
		}

		// draw buttons
		button_draw(state,&buttonnew);
		button_draw(state,&buttonstop);

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

