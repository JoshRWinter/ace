#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <android_native_app_glue.h>
#include <stdlib.h>
#include <math.h>
#include "defs.h"

int menu_main(struct state *state){
	struct button buttonconf={{-6.75f,3.0f,BUTTON_WIDTH,BUTTON_HEIGHT,0.0f,1.0f,0.0f},"Settings",false};
	struct button buttonaboot={{-3.25f,3.0f,BUTTON_WIDTH,BUTTON_HEIGHT,0.0f,1.0f,0.0f},"Aboot",false};
	struct button buttonplay={{0.25f,3.0f,BUTTON_WIDTH,BUTTON_HEIGHT,0.0f,1.0f,0.0f},"Play",false};
	struct button buttonquit={{3.75f,3.0f,BUTTON_WIDTH,BUTTON_HEIGHT,0.0f,1.0f,0.0f},"Quit",false};

	struct base title={state->rect.left,state->rect.top,state->rect.right*2.0f,state->rect.bottom*2.0f,0.0f,1.0f,0.0f};
	const char *aboot=
		"~ ACE ~ \n"
		"Programming and Art by Josh Winter\n"
		"yadda yadda";
	float yoff=0.0f;
	float slide=0.0f;
	const float inc=0.01f;
	int transition=false;

	// generate lots o clouds
	for(int i=0;i<18;++i){
		for(int j=0;j<1;++j)
			newlargecloud(state,true);
		for(struct largecloud *cloud=state->largecloudlist;cloud!=NULL;cloud=cloud->next){
			const float MULTIPLIER=randomint(10,15);
			cloud->base.x+=cloud->xv*MULTIPLIER;
			cloud->base.y+=cloud->yv*MULTIPLIER;
		}
	}

	while(process(state->app)){
		buttonplay.base.y=yoff+3.0f;
		buttonaboot.base.y=buttonplay.base.y;
		buttonconf.base.y=buttonplay.base.y;
		buttonquit.base.y=buttonplay.base.y;
		title.y=yoff+state->rect.top;

		glClear(GL_COLOR_BUFFER_BIT);

		// process large clouds
		if(onein(45))
			newlargecloud(state,false);
		glBindTexture(GL_TEXTURE_2D,state->uiassets.texture[TID_LARGECLOUD].object);
		for(struct largecloud *cloud=state->largecloudlist,*prevcloud=NULL;cloud!=NULL;){
			if(cloud->base.x>state->rect.right){
				cloud=deletelargecloud(state,cloud,prevcloud);
				continue;
			}
			cloud->base.x+=cloud->xv;
			cloud->base.y=yoff+cloud->base.y;

			if(!cloud->top){
				// render
				uidraw(state,&cloud->base);
			}

			prevcloud=cloud;
			cloud=cloud->next;
		}

		// draw background
		glUniform4f(state->uniform.rgba,1.0f,1.0f,1.0f,1.0f);
		glBindTexture(GL_TEXTURE_2D,state->uiassets.texture[TID_TITLE].object);
		draw(state,&title);

		// buttons
		if(button_process(state->pointer,&buttonplay)==BUTTON_ACTIVATE){
			transition=true;
		}
		if(button_process(state->pointer,&buttonaboot)==BUTTON_ACTIVATE){
			if(!menu_message(state,"Aboot",aboot))
				return false;
		}
		if(button_process(state->pointer,&buttonconf)==BUTTON_ACTIVATE){
			if(!menu_conf(state))
				return false;
		}
		if(button_process(state->pointer,&buttonquit)==BUTTON_ACTIVATE||state->back){
			state->back=false;
			ANativeActivity_finish(state->app->activity);
		}

		// draw buttons
		button_draw(state,&buttonplay);
		button_draw(state,&buttonaboot);
		button_draw(state,&buttonconf);
		button_draw(state,&buttonquit);

		// process large clouds
		glUniform4f(state->uniform.rgba,1.0f,1.0f,1.0f,1.0f);
		glBindTexture(GL_TEXTURE_2D,state->uiassets.texture[TID_LARGECLOUD].object);
		for(struct largecloud *cloud=state->largecloudlist;cloud!=NULL;cloud=cloud->next)
			if(cloud->top)
				uidraw(state,&cloud->base);

		// handle transition animation
		if(transition){
			slide+=inc;
			yoff+=slide;
			if(title.y>state->rect.bottom){
				for(struct largecloud *cloud=state->largecloudlist;cloud!=NULL;cloud=deletelargecloud(state,cloud,NULL));
				return menu_transition(state);
			}
		}

		eglSwapBuffers(state->display,state->surface);
	}
	return false;
}

int menu_conf(struct state *state){
	const float rmargin=2.0f;
	struct button buttonvib={{rmargin,-2.0f,BUTTON_WIDTH,BUTTON_HEIGHT,0.0f,1.0f,0.0f},"Vibrate",false};
	struct button buttonsound={{rmargin,-0.5f,BUTTON_WIDTH,BUTTON_HEIGHT,0.0f,1.0f,0.0f},"Sounds",false};
	struct button buttonmusic={{rmargin,1.0f,BUTTON_WIDTH,BUTTON_HEIGHT,0.0f,1.0f,0.0f},"Music",false};
	struct button buttonback={{-BUTTON_WIDTH/2.0f,3.0f,BUTTON_WIDTH,BUTTON_HEIGHT,0.0f,1.0f,0.0f},"Back",false};
	char status[76];
	int changed=false;
	while(process(state->app)){
		glClear(GL_COLOR_BUFFER_BIT);

		glUniform4f(state->uniform.rgba,1.0f,1.0f,1.0f,1.0f);
		// header
		glBindTexture(GL_TEXTURE_2D,state->font.header->atlas);
		drawtextcentered(state->font.header,0.0f,-4.0f,"Configuration");

		// status line
		glBindTexture(GL_TEXTURE_2D,state->font.main->atlas);
		sprintf(status,"Vibration: %s\nSounds: %s\nMusic: %s",state->vibrate?"yes":"no",state->sounds?"yes":"no",state->music?"yes":"no");
		drawtextcentered(state->font.main,-3.0f,-0.75f,status);

		// buttons
		if(button_process(state->pointer,&buttonvib)==BUTTON_ACTIVATE){
			state->vibrate=!state->vibrate;
			changed=true;
		}
		if(button_process(state->pointer,&buttonsound)==BUTTON_ACTIVATE){
			state->sounds=!state->sounds;
			changed=true;
		}
		if(button_process(state->pointer,&buttonmusic)==BUTTON_ACTIVATE){
			state->music=!state->music;
			changed=true;
		}
		if(button_process(state->pointer,&buttonback)==BUTTON_ACTIVATE||state->back){
			state->back=false;
			if(changed)
				save_settings(state);
			return true;
		}

		// draw buttons
		button_draw(state,&buttonvib);
		button_draw(state,&buttonsound);
		button_draw(state,&buttonmusic);
		button_draw(state,&buttonback);

		eglSwapBuffers(state->display,state->surface);
	}
	return false;
}

int menu_pause(struct state *state){
	struct button buttonresume={{2.0f,3.0,BUTTON_WIDTH,BUTTON_HEIGHT,0.0f,1.0f,0.0f},"Resume",false};
	struct button buttonmenu={{-BUTTON_WIDTH/2.0f,3.0,BUTTON_WIDTH,BUTTON_HEIGHT,0.0f,1.0f,0.0f},"Menu",false};
	struct button buttonconf={{-5.0f,3.0f,BUTTON_WIDTH,BUTTON_HEIGHT,0.0f,1.0f,0.0f},"Settings",false};
	while(process(state->app)){
		//glClear(GL_COLOR_BUFFER_BIT);
		render(state);
		glUniform4f(state->uniform.rgba,1.0f,1.0f,1.0f,1.0f);

		// buttons
		if(button_process(state->pointer,&buttonresume)==BUTTON_ACTIVATE){
			return true;
		}
		if(button_process(state->pointer,&buttonmenu)==BUTTON_ACTIVATE){
			reset(state);
			state->showmenu=true;
			return core(state);
		}
		if(button_process(state->pointer,&buttonconf)==BUTTON_ACTIVATE){
			if(!menu_conf(state))
				return false;
		}

		glBindTexture(GL_TEXTURE_2D,state->font.header->atlas);
		drawtextcentered(state->font.header,0.0f,-3.0f,"PAUSED");

		// draw buttons
		button_draw(state,&buttonresume);
		button_draw(state,&buttonmenu);
		button_draw(state,&buttonconf);

		if(state->back){
			state->back=false;
			return true;
		}

		eglSwapBuffers(state->display,state->surface);
	}
	return false;
}

int menu_end(struct state *state){
	struct button buttonnew={{0.25f,3.0f,BUTTON_WIDTH,BUTTON_HEIGHT,0.0f,1.0f,0.0f},"Again",false};
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
	float slideout=vd;
	float yoff=state->rect.top*2.0f;
	int transition=false; // transition out
	// compute and sort highscores
	int newhighscore=-1;
	if((int)state->points>state->highscore[0]){
		state->highscore[0]=state->points;
		selection(state->highscore);
		// take note of the index
		for(int i=0;i<HIGHSCORE_COUNT;++i){
			if(state->highscore[i]==(int)state->points){
				newhighscore=i;
				break;
			}
		}
		save_highscores(state);
	}

	// award logic
	if(newhighscore==HIGHSCORE_COUNT-1&&state->points>300)
		if(!menu_award(state,AWARD_DFC))
			return false;

	while(process(state->app)){
		if(!transition){
			for(struct enemy *enemy=state->enemylist;enemy!=NULL;enemy=enemy->next)
				enemy->base.y+=slide;
			for(struct group *group=state->grouplist;group!=NULL;group=group->next)
				group->base.y+=slide;
			for(struct bomb *bomb=state->bomblist;bomb!=NULL;bomb=bomb->next)
				bomb->base.y+=slide;
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
		}

		if(!transition){
			slide+=vd;
			render(state);
		}
		else
			glClear(GL_COLOR_BUFFER_BIT);

		if(slide>0.5f){
			buttonnew.base.y=yoff+3.0f;
			buttonstop.base.y=yoff+3.0f;

			if(!transition){
				if(yoff!=0.0f){
					yoff-=(yoff/10.0f);
					if(yoff>-0.005)
						yoff=0.0f;
				}
			}

			// header
			glUniform4f(state->uniform.rgba,1.0f,1.0f,1.0f,1.0f);
			glBindTexture(GL_TEXTURE_2D,state->font.header->atlas);
			drawtextcentered(state->font.header,0.0f,yoff+-4.0f,egg?"lol u ded":"KILLED IN ACTION");

			// end game stats
			glBindTexture(GL_TEXTURE_2D,state->font.button->atlas);
			drawtextcentered(state->font.button,0.0f,yoff-2.5f,info);

			// highscores
			glBindTexture(GL_TEXTURE_2D,state->font.main->atlas);
			char listing[16];
			for(int i=HIGHSCORE_COUNT-1;i>=0;--i){
				float offset=yoff-1.0f+((HIGHSCORE_COUNT-1-i)*0.41f);
				if(state->highscore[i]==0)
					sprintf(listing,"%d. -",HIGHSCORE_COUNT-i);
				else
					sprintf(listing,"%d. %d",HIGHSCORE_COUNT-i,state->highscore[i]);
				if(newhighscore==i){
					glUniform4f(state->uniform.rgba,HIGHSCORE_HIGHLIGHT);
					drawtextcentered(state->font.main,0.0f,offset,listing);
					glUniform4f(state->uniform.rgba,1.0f,1.0f,1.0f,1.0f);
				}
				else
					drawtextcentered(state->font.main,0.0f,offset,listing);
			}

			// buttons
			if(button_process(state->pointer,&buttonnew)==BUTTON_ACTIVATE){
				reset(state);
				transition=true;
			}
			if(button_process(state->pointer,&buttonstop)==BUTTON_ACTIVATE||state->back){
				state->back=false;
				reset(state);
				state->showmenu=true;
				return true;
			}

			// draw buttons
			button_draw(state,&buttonnew);
			button_draw(state,&buttonstop);

			// handle transition animation (out transition)
			if(transition){
				slideout+=vd;
				yoff+=slideout;
				if(yoff>8.0f)
					return menu_transition(state);
			}
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

		if(state->back){
			state->back=false;
			return true;
		}

		eglSwapBuffers(state->display,state->surface);
	}
	return false;
}

int menu_transition(struct state *state){
	struct base player={-PLAYER_WIDTH/2.0f,state->rect.top-PLAYER_HEIGHT,PLAYER_WIDTH,PLAYER_HEIGHT,0.0f,1.0f,0.0f};
	while(process(state->app)){
		glUniform4f(state->uniform.rgba,1.0f,1.0f,1.0f,1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		float step=(fabs(player.y+PLAYER_HEIGHT/2.0f)/10.0f)+0.0005f;
		if(targetf(&player.y,step,-PLAYER_HEIGHT/2.0f)==-PLAYER_HEIGHT/2.0f)
			return true;

		glBindTexture(GL_TEXTURE_2D,state->assets.texture[TID_PLAYER].object);
		draw(state,&player);

		eglSwapBuffers(state->display,state->surface);
	}
	return false;
}

int menu_award(struct state *state,int award){
	char *message;
	int tid;
	struct base awddfc={-DFC_WIDTH/2.0f,-1.0f,DFC_WIDTH,DFC_HEIGHT,0.0f,1.0f,0.0f};
	struct base *award_base;
	struct button buttonok={{-BUTTON_WIDTH/2.0f,3.0f,BUTTON_WIDTH,BUTTON_HEIGHT,0.0f,1.0f,0.0f},"OK",false};
	switch(award){
	case AWARD_DFC:
		message="The United States Navy is proud to present this\n"
			"Distinguished Flying Cross\n"
			"For your heroic actions against aerial enemy combatants\n"
			"on this day, February 2, 1945";
			tid=TID_AWDDFC;
			award_base=&awddfc;
			break;
	}
	while(process(state->app)){
		glClear(GL_COLOR_BUFFER_BIT);

		glUniform4f(state->uniform.rgba,1.0f,1.0f,1.0f,1.0f);
		// message
		glBindTexture(GL_TEXTURE_2D,state->font.main->atlas);
		drawtextcentered(state->font.main,0.0f,-3.75f,message);

		// award
		glBindTexture(GL_TEXTURE_2D,state->uiassets.texture[TID_AWDDFC].object);
		uidraw(state,award_base);

		// button
		if(button_process(state->pointer,&buttonok)==BUTTON_ACTIVATE||state->back){
			state->back=false;
			return true;
		}

		// draw button
		button_draw(state,&buttonok);

		eglSwapBuffers(state->display,state->surface);
	}
	return false;
}

