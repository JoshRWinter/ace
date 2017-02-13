#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <android_native_app_glue.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include "defs.h"

int core(struct state *state){
	char msg[MESSAGE_MAX+1]; // used in various places to show a message
	if(state->showmenu){
		state->showmenu=false;
		if(!menu_main(state))
			return false;
	}

	// proc clouds
	newcloud(state);
	for(struct cloud *cloud=state->cloudlist,*prevcloud=NULL;cloud!=NULL;){
		if(distance(cloud->base.x+(CLOUD_SIZE/2.0f),state->player.base.x+(PLAYER_WIDTH/2.0f),
		cloud->base.y+(CLOUD_SIZE/2.0f),state->player.base.y+(PLAYER_WIDTH/2.0f))>CLOUD_RMDIST){
				cloud=deletecloud(state,cloud,prevcloud);
				continue;
		}
		// parallax effect
		if(!state->player.dead){
			cloud->base.x-=state->player.xv*cloud->parallax;
			cloud->base.y-=state->player.yv*cloud->parallax;
		}

		prevcloud=cloud;
		cloud=cloud->next;
	}

	// proc enemies
	if(onein(140)||state->enemylist==NULL)newenemy(state);
	if(onein(300)&&state->focused_enemy==NULL){
		// choose an enemy at random to be the "focused" enemy
		while(state->enemylist&&state->focused_enemy==NULL){
			for(struct enemy *enemy=state->enemylist;enemy!=NULL;enemy=enemy->next){
				if(onein(5)){
					state->focused_enemy=enemy;
					break;
				}
			}
		}
	}
	for(struct enemy *enemy=state->enemylist,*prevenemy=NULL;enemy!=NULL;){
		if(enemy->dead){
			enemy=deleteenemy(state,enemy,prevenemy);
			continue;
		}
		if(enemy->dying==0.0f&&(state->missilelist==NULL?onein(750):onein(1550))&&!state->player.dead&&state->focused_enemy==NULL)newmissile(state,enemy);
		float angle=atan2f((enemy->base.y+(ENEMY_HEIGHT/2.0f))-enemy->target.y,
				(enemy->base.x+(ENEMY_WIDTH/2.0f))-enemy->target.x);
		align(&enemy->base.rot,enemy->dying>0.0f?(PLAYER_TURN_SPEED*1.5f*state->gamespeed):(PLAYER_TURN_SPEED*state->gamespeed),angle);
		enemy->xv=-cosf(enemy->base.rot)*(state->focused_enemy==enemy?PLAYER_SPEED-0.02f:ENEMY_SPEED);
		enemy->yv=-sinf(enemy->base.rot)*(state->focused_enemy==enemy?PLAYER_SPEED-0.02f:ENEMY_SPEED);
		enemy->base.x+=enemy->xv*state->gamespeed;
		enemy->base.y+=enemy->yv*state->gamespeed;

		// decide whether to fire upon the player
		if(enemy->dying==0.0f&&state->missilelist==NULL&&distance(enemy->base.x+(ENEMY_WIDTH/2.0f),state->player.base.x+(PLAYER_WIDTH/2.0f),
					enemy->base.y+(ENEMY_HEIGHT/2.0f),state->player.base.y+(PLAYER_HEIGHT/2.0f))<ENEMY_FIRE_DIST){
			float cone=atan2f((enemy->base.y+(ENEMY_HEIGHT/2.0f))-(state->player.base.y+(PLAYER_HEIGHT/2.0f)),
					(enemy->base.x+(ENEMY_WIDTH/2.0f))-(state->player.base.x+(PLAYER_WIDTH/2.0f)));
			while(cone<0.0f)cone+=M_PI*2.0f;
			while(cone>M_PI*2.0f)cone-=M_PI*2.0f;
			while(enemy->base.rot<0.0f)enemy->base.rot+=M_PI*2.0f;
			while(enemy->base.rot>M_PI*2.0f)enemy->base.rot-=M_PI*2;
			float diff=enemy->base.rot-cone;
			if(diff>M_PI)diff=(M_PI*2.0f)-diff;
			if(fabs(diff)<ENEMY_CONE&&enemy->timer_reload<=0.0f){
				if(onscreen(enemy)&&state->sounds)
					playsound(state->soundengine,state->aassets.sound+SID_FIRE,false);
				newbullet(state,&enemy->base);
				enemy->timer_reload=PLAYER_RELOAD;
			}
		}
		if(enemy->timer_reload>0.0f)
			enemy->timer_reload-=state->gamespeed;

		if(enemy->dying>0.0f){
			// enemy has been shot down
			// spin out of control briefly before blowing up
			const float aoff=3.0f;
			enemy->target.x=(enemy->base.x+(ENEMY_WIDTH/2.0f))-cosf(enemy->base.rot+aoff)/5.0f;
			enemy->target.y=(enemy->base.y+(ENEMY_HEIGHT/2.0f))-sinf(enemy->base.rot+aoff)/5.0f;
			if(enemy->dying>ENEMY_SPIN_TIMER){
				if(onscreen(enemy)&&state->sounds)
					playsound(state->soundengine,state->aassets.sound+SID_EXPLOSION,false);
				newexplosion(state,enemy->base.x+(ENEMY_WIDTH/2.0f),enemy->base.y+(ENEMY_HEIGHT/2.0f),0.32f,false);
				enemy=deleteenemy(state,enemy,prevenemy);
				continue;
			}
			else
				enemy->dying+=state->gamespeed;
		}
		else if(enemy!=state->focused_enemy){
			// find new random waypoint
			if(collide(&enemy->base,&enemy->target,0.0f)){
				enemy->target.x=randomint((state->player.base.x-17.0f)*10.0f,(state->player.base.x+17.0f)*10.0f)/10.0f;
				enemy->target.y=randomint((state->player.base.y-17.0f)*10.0f,(state->player.base.y+17.0f)*10.0f)/10.0f;
			}
		}
		else{
			// chase the player in a dogfight
			enemy->target.x=state->player.base.x+(PLAYER_WIDTH/2.0f);
			enemy->target.y=state->player.base.y+(PLAYER_HEIGHT/2.0f);
			if(onein(190))state->focused_enemy=NULL;
		}

		// check for enemies colliding with player
		if(enemy->dying==0.0f&&!state->player.dead&&collide(&state->player.base,&enemy->base,0.5f)){
			state->player.dead=true;
			if(state->vibrate)vibratedevice(&state->jni_info,DEATH_RATTLE);
			float x1=state->player.base.x+(PLAYER_WIDTH/2.0f);
			float x2=enemy->base.x+(ENEMY_WIDTH/2.0f);
			float y1=state->player.base.y+(PLAYER_HEIGHT/2.0f);
			float y2=enemy->base.y+(ENEMY_HEIGHT/2.0f);
			if(state->sounds)
				playsound(state->soundengine,state->aassets.sound+SID_EXPLOSION,false);
			newexplosion(state,(x1+x2)/2.0f,(y1+y2)/2.0f,0.5f,false);
			enemy=deleteenemy(state,enemy,prevenemy);
			continue;
		}

		// check for enemies colliding with other enemies
		for(struct enemy *enemy2=state->enemylist;enemy2!=NULL;enemy2=enemy2->next){
			if(enemy==enemy2)
				continue;
			if(collide(&enemy->base,&enemy2->base,0.3f)){
				enemy->dead=true;
				enemy2->dead=true;
				float x1=enemy2->base.x+(ENEMY_WIDTH/2.0f);
				float x2=enemy->base.x+(ENEMY_WIDTH/2.0f);
				float y1=enemy2->base.y+(ENEMY_HEIGHT/2.0f);
				float y2=enemy->base.y+(ENEMY_HEIGHT/2.0f);
				if(onscreen(enemy)&&state->sounds)
					playsound(state->soundengine,state->aassets.sound+SID_EXPLOSION,false);
				newexplosion(state,(x1+x2)/2.0f,(y1+y2)/2.0f,0.5f,false);
			}
		}

		if(enemy->timer_smoke<=0.0f){
			enemy->timer_smoke=PLAYER_SMOKE;
			newsmoke(state,&enemy->base,0.4f,0.3f,enemy->health/100.0f);
		}
		else enemy->timer_smoke-=enemy->dying?state->gamespeed*3.75f:state->gamespeed;
		
		prevenemy=enemy;
		enemy=enemy->next;
	}

	// proc groups
	int active_groups=0;
	for(struct group *group=state->grouplist;group!=NULL;group=group->next){
		if(!group->dead)
			++active_groups;
		if(!state->player.dead){
			group->base.x+=state->player.xv/GROUP_DEPTH;
			group->base.y+=state->player.yv/GROUP_DEPTH;
		}
		if(group->dead&&group->timer_explosions>-45.0f){
			group->timer_explosions-=state->gamespeed;
			if(((int)group->timer_explosions)%10==0){
				float xoff=randomint(-10,10)/10.0f;
				float yoff=randomint(-10,10)/10.0f;
				newexplosion(state,group->base.x+(GROUP_WIDTH/2.0f)+xoff,group->base.y+(GROUP_HEIGHT/2.0f)+yoff,0.4f,true);
			}
		}
	}
	if(active_groups<1&&onein(800))newgroup(state);

	// proc bombs
	if(state->bomb&&state->player.timer_bomb<=0.0f){
		if(state->sounds)
			playsound(state->soundengine,state->aassets.sound+SID_DROP,false);
		state->player.bombs=BOMB_COUNT;
		state->player.timer_bomb=BOMB_RECHARGE;
	}
	else if(state->player.timer_bomb>0.0f)--state->player.timer_bomb;
	if(state->player.bombs&&--state->player.bombs%4==0)newbomb(state);
	for(struct bomb *bomb=state->bomblist,*prevbomb=NULL;bomb!=NULL;){
		bomb->xv*=BOMB_DRAG;
		bomb->yv*=BOMB_DRAG;
		bomb->base.x+=bomb->xv*state->gamespeed;
		bomb->base.y+=bomb->yv*state->gamespeed;
		if(!state->player.dead){
			bomb->base.x+=state->player.xv/bomb->depth;
			bomb->base.y+=state->player.yv/bomb->depth;
			if(bomb->depth<GROUP_DEPTH)
				bomb->depth=GROUP_DEPTH;
			bomb->depth-=0.02f;
		}
		const float BOMB_SHRINK=0.97f;
		float oldw=bomb->base.w;
		float oldh=bomb->base.h;
		bomb->base.w*=BOMB_SHRINK;
		bomb->base.h*=BOMB_SHRINK;
		bomb->base.x+=(oldw-bomb->base.w)/1.0f;
		bomb->base.y+=(oldh-bomb->base.h)/1.0f;
		if(!--bomb->altitude){
			// check for bombs colliding with carrier groups
			int stop=false;
			for(struct group *group=state->grouplist,*prevgroup=NULL;group!=NULL;){
				if(collide(&bomb->base,&group->base,0.0f)){
					if(!group->dead&&(group->health-=6)<1){
						group->dead=true;
						if(state->sounds)
							playsound(state->soundengine,state->aassets.sound+SID_HUGEEXPLOSION,false);
						newexplosion(state,group->base.x+(GROUP_WIDTH/2.0f),group->base.y+(GROUP_HEIGHT/2.0f),0.5f,true);
						state->points+=POINTS_GROUP_DESTROYED;
						sprintf(msg,"+%d carrier group destroyed",POINTS_GROUP_DESTROYED);
						newmessage(state,msg);
						if(state->vibrate)vibratedevice(&state->jni_info,CARRIER_RATTLE);
					}
					newexplosion(state,bomb->base.x,bomb->base.y,0.05f,true);
					if(state->sounds)
						playsound(state->soundengine,state->aassets.sound+SID_DISTEXPLOSION,false);
					bomb=deletebomb(state,bomb,prevbomb);
					stop=true;
					break;
				}
				prevgroup=group;
				group=group->next;
			}
			if(stop)continue;
			bomb=deletebomb(state,bomb,prevbomb);
			continue;
		}
		prevbomb=bomb;
		bomb=bomb->next;
	}

	// proc missiles
	for(struct missile *missile=state->missilelist,*prevmissile=NULL;missile!=NULL;){
		if(missile->dead){
			missile=deletemissile(state,missile,prevmissile);
			continue;
		}
		if((missile->ttl-=state->gamespeed)<-32.0f){
			if(onscreen(missile)&&state->sounds)
				playsound(state->soundengine,state->aassets.sound+SID_SMALLEXPLOSION,false);
			newexplosion(state,missile->base.x+(MISSILE_WIDTH/2.0f),missile->base.y+(MISSILE_HEIGHT/2.0f),0.2f,false);
			missile=deletemissile(state,missile,prevmissile);
			continue;
		}
		// unfocus any enemies
		state->focused_enemy=NULL;
		// calculate missile sway (weaving back and forth)
		float dist=distance(missile->base.x+(MISSILE_WIDTH/2.0f),state->player.base.x+(PLAYER_WIDTH/2.0f),
				missile->base.y+(MISSILE_HEIGHT/2.0f),state->player.base.x+(PLAYER_WIDTH/2.0f));
		if(dist>4.0f)dist=4.0f;
		missile->sway+=0.1f*state->gamespeed;
		float sway=(sin(missile->sway)/15.0f)*(dist/1.5f);
		float angle;
		if(state->player.dead){
			if(missile->ttl>0)missile->ttl=-25;
			angle=missile->base.rot+(M_PI/2.0f);
			missile->xv=-cosf(missile->base.rot)*MISSILE_SPEED;
			missile->yv=-sinf(missile->base.rot)*MISSILE_SPEED;
		}
		else if(missile->ttl>0){
			angle=atan2f((missile->base.y+(MISSILE_HEIGHT/2.0f))-(state->player.base.y+(PLAYER_HEIGHT/2.0f)),
				(missile->base.x+(MISSILE_WIDTH/2.0f))-(state->player.base.x+(PLAYER_WIDTH/2.0f)))+sway;
			missile->xv=-cosf(missile->base.rot)*MISSILE_SPEED;
			missile->yv=-sinf(missile->base.rot)*MISSILE_SPEED;
		}
		else if(missile->ttl==0){
			if(!state->player.dead){
				sprintf(msg,"+%d missile evaded",POINTS_MISSILE_EVADED);
				newmessage(state,msg);
				state->points+=POINTS_MISSILE_EVADED;
			}
		}
		else{
			// missile timed out
			angle=missile->base.rot;
			missile->xv/=1.02f;
			missile->yv/=1.02f;
		}
		align(&missile->base.rot,MISSILE_TURN_SPEED*state->gamespeed,angle);
		missile->base.x+=missile->xv*state->gamespeed;
		missile->base.y+=missile->yv*state->gamespeed;
		// check for missiles colliding with other missiles
		for(struct missile *missile2=state->missilelist;missile2!=NULL;missile2=missile2->next){
			if(missile==missile2)
				continue;
			if(collide(&missile->base,&missile2->base,0.05f)){
				if(!state->player.dead){
					sprintf(msg,"+%d missiles collided",POINTS_MISSILES_COLLIDE);
					newmessage(state,msg);
					state->points+=POINTS_MISSILES_COLLIDE;
				}

				missile2->dead=true;
				missile->dead=true;
				float x1=missile->base.x+(MISSILE_WIDTH/2.0f);
				float x2=missile2->base.x+(MISSILE_WIDTH/2.0f);
				float y1=missile->base.y+(MISSILE_HEIGHT/2.0f);
				float y2=missile2->base.y+(MISSILE_HEIGHT/2.0f);
				if(onscreen(missile)&&state->sounds)
					playsound(state->soundengine,state->aassets.sound+SID_SMALLEXPLOSION,false);
				newexplosion(state,(x1+x2)/2.0f,(y1+y2)/2.0f,0.2f,false);
			}
		}
		// check for missiles colliding with player
		if(!state->player.dead&&collide(&missile->base,&state->player.base,PLAYER_TOLERANCE)){
			if(onein(3)){
				if((state->player.health-=85)<1)
					state->player.dead=true;
				else
					newexplosion(state,missile->base.x+(MISSILE_WIDTH/2.0f),missile->base.y+(MISSILE_HEIGHT/2.0f),0.2f,false);
			}
			else
				state->player.dead=true;

			if(state->player.dead){
				if(state->sounds)
					playsound(state->soundengine,state->aassets.sound+SID_EXPLOSION,false);
				newexplosion(state,state->player.base.x+(PLAYER_WIDTH/2.0f),state->player.base.y+(PLAYER_HEIGHT/2.0f),0.3f,false);
			}
			else if(state->sounds)
				playsound(state->soundengine,state->aassets.sound+SID_SMALLEXPLOSION,false);

			missile=deletemissile(state,missile,prevmissile);
			if(state->vibrate)vibratedevice(&state->jni_info,DEATH_RATTLE);
			continue;
		}
		// check for missiles colliding with enemies
		int stop=false;
		if(MISSILE_TTL-missile->ttl>20)
			for(struct enemy *enemy=state->enemylist,*prevenemy=NULL;enemy!=NULL;){
				if(collide(&missile->base,&enemy->base,0.1f)){
					newexplosion(state,enemy->base.x+(ENEMY_WIDTH/2.0f),enemy->base.y+(ENEMY_HEIGHT/2.0f),0.3f,false);
					if(onscreen(enemy)&&state->sounds)
						playsound(state->soundengine,state->aassets.sound+SID_EXPLOSION,false);
					enemy=deleteenemy(state,enemy,prevenemy);
					missile=deletemissile(state,missile,prevmissile);
					stop=true;
					break;
				}
				prevenemy=enemy;
				enemy=enemy->next;
			}
		if(stop)continue;
		if(missile->timer_smoke<=0.0f){
			missile->timer_smoke=MISSILE_SMOKE;
			newsmoke(state,&missile->base,0.2f,0.3f,1.0f);
		}
		else missile->timer_smoke-=state->gamespeed;

		prevmissile=missile;
		missile=missile->next;
	}

	// proc bullets
	if(state->fire&&state->player.reload<=0&&!state->player.dead){
		if(state->sounds)
			playsound(state->soundengine,state->aassets.sound+SID_FIRE,false);
		newbullet(state,&state->player.base);
		state->player.reload=PLAYER_RELOAD;
	}
	for(struct bullet *bullet=state->bulletlist,*prevbullet=NULL;bullet!=NULL;){
		// delete bullets that go too far off screen
		if(bullet->base.x<state->player.base.x-20.0f||bullet->base.x>state->player.base.x+20.0f||
				bullet->base.y<state->player.base.y-20.0f||state->player.base.y>state->player.base.y+20.0f){
			bullet=deletebullet(state,bullet,prevbullet);
			continue;
		}
		// check for bullets colliding with missiles
		int stop=false;
		for(struct missile *missile=state->missilelist,*prevmissile=NULL;missile!=NULL;){
			if(bullet->owner==&state->player.base&&collide(&bullet->base,&missile->base,-0.15f)){
				if(!state->player.dead){
					sprintf(msg,"+%d missile shot down",POINTS_MISSILE_SHOT_DOWN);
					newmessage(state,msg);
					state->points+=POINTS_MISSILE_SHOT_DOWN;
				}

				if(onscreen(bullet)&&state->sounds)
					playsound(state->soundengine,state->aassets.sound+SID_SMALLEXPLOSION,false);
				newexplosion(state,missile->base.x+(MISSILE_WIDTH/2.0f),missile->base.y+(MISSILE_HEIGHT/2.0f),0.2f,false);
				missile=deletemissile(state,missile,prevmissile);
				bullet=deletebullet(state,bullet,prevbullet);
				stop=true;
				break;
			}
			prevmissile=missile;
			missile=missile->next;
		}
		if(stop)continue;
		// check for bullets colliding with enemies
		for(struct enemy *enemy=state->enemylist,*prevenemy=NULL;enemy!=NULL;){
			if(bullet->owner!=&enemy->base&&collide(&bullet->base,&enemy->base,0.1f)){
				if(onscreen(enemy)&&state->sounds)
					playsound(state->soundengine,state->aassets.sound+SID_HIT,false);
				newexplosion(state,bullet->base.x+(BULLET_WIDTH/2.0f),bullet->base.y+(BULLET_HEIGHT/2.0f),0.05,false);
				if(enemy->dying==0.0f&&(enemy->health-=randomint(BULLET_DMG))<1){
					if(!state->player.dead&&bullet->owner==&state->player.base){
						++state->player.victories; // arial victory
						if(state->player.health<50&&onein(2))newhealth(state,enemy);
						sprintf(msg,"+%d enemy shot down",POINTS_ENEMY_SHOT_DOWN);
						newmessage(state,msg);
						state->points+=POINTS_ENEMY_SHOT_DOWN;
					}

					int spin_out=onein(3); // have the enemy spin out of control before exploding
					if(spin_out){
						enemy->dying+=state->gamespeed;
						newexplosion(state,enemy->base.x+(ENEMY_WIDTH/2.0f),enemy->base.y+(ENEMY_HEIGHT/2.0f),0.2f,false);
						if(onscreen(enemy)&&state->sounds)
							playsound(state->soundengine,state->aassets.sound+SID_SMALLEXPLOSION,false);
					}
					else{
						newexplosion(state,enemy->base.x+(ENEMY_WIDTH/2.0f),enemy->base.y+(ENEMY_HEIGHT/2.0f),0.32f,false);
						if(onscreen(enemy)&&state->sounds)
							playsound(state->soundengine,state->aassets.sound+SID_EXPLOSION,false);
						enemy=deleteenemy(state,enemy,prevenemy);
					}
				}
				bullet=deletebullet(state,bullet,prevbullet);
				stop=true;
				break;
			}
			prevenemy=enemy;
			enemy=enemy->next;
		}
		if(stop)continue;
		// check for bullets colliding with player
		if(!state->player.dead&&bullet->owner!=&state->player.base&&collide(&bullet->base,&state->player.base,0.1f)){
			if(state->vibrate)vibratedevice(&state->jni_info,HIT_RATTLE);
			if(state->sounds)
				playsound(state->soundengine,state->aassets.sound+SID_HIT,false);
			state->player.health-=randomint(BULLET_ENEMY_DMG);
			if(state->player.health<1){
				if(state->sounds)
					playsound(state->soundengine,state->aassets.sound+SID_EXPLOSION,false);
				newexplosion(state,state->player.base.x+(PLAYER_WIDTH/2.0f),state->player.base.y+(PLAYER_HEIGHT/2.0f),0.3f,false);
				state->player.dead=true;
				if(state->vibrate)vibratedevice(&state->jni_info,DEATH_RATTLE);
			}
			newexplosion(state,bullet->base.x+(BULLET_WIDTH/2.0f),bullet->base.y+(BULLET_HEIGHT/2.0f),0.05,false);
			bullet=deletebullet(state,bullet,prevbullet);
			continue;
		}

		bullet->base.x+=bullet->xv*state->gamespeed;
		bullet->base.y+=bullet->yv*state->gamespeed;
		prevbullet=bullet;
		bullet=bullet->next;
	}

	// proc smoke
	for(struct smoke *smoke=state->smokelist,*prevsmoke=NULL;smoke!=NULL;){
		smoke->base.x+=smoke->xv*state->gamespeed;
		smoke->base.y+=smoke->yv*state->gamespeed;
		smoke->alpha-=SMOKE_FADE*state->gamespeed;
		if(smoke->alpha<=0.0f){
			smoke=deletesmoke(state,smoke,prevsmoke);
			continue;
		}
		smoke->base.w-=SMOKE_SHRINK*state->gamespeed;
		smoke->base.h=smoke->base.w;
		smoke->base.x+=(SMOKE_SHRINK*state->gamespeed)/2.0f;
		smoke->base.y+=(SMOKE_SHRINK*state->gamespeed)/2.0f;
		prevsmoke=smoke;
		smoke=smoke->next;
	}

	// proc health
	for(struct health *health=state->healthlist,*prevhealth=NULL;health!=NULL;){
		health->base.x+=health->xv*state->gamespeed;
		health->base.y+=health->yv*state->gamespeed;
		const float RETARD=0.987f;
		health->xv*=RETARD;
		health->yv*=RETARD;
		health->base.rot+=health->xv;
		if(collide(&state->player.base,&health->base,0.2f)){
			health=deletehealth(state,health,prevhealth);
			state->player.health=100;
			newmessage(state,"full repair");
			continue;
		}
		prevhealth=health;
		health=health->next;
	}

	// proc explosions
	for(struct explosion *explosion=state->explosionlist,*prevexplosion=NULL;explosion!=NULL;){
		int done=true; // explosion is ready to be deleted
		for(int i=0;i<EXPLOSION_FLASH_COUNT;++i){
			if(!state->player.dead&&explosion->sealevel){
				explosion->flash[i].base.x+=state->player.xv/GROUP_DEPTH;
				explosion->flash[i].base.y+=state->player.yv/GROUP_DEPTH;
			}
			if(explosion->flash[i].timer_delay<=0.0f){
				float increment=((EXPLOSION_FLASH_MAX_GROW_RATE*fabs(explosion->flash[i].maxsize-explosion->flash[i].base.w))+0.005f)*state->gamespeed;
				if(explosion->flash[i].growing){
					done=false;
					explosion->flash[i].base.w+=increment;
					if(explosion->flash[i].base.w>explosion->flash[i].maxsize){
						explosion->flash[i].base.w=explosion->flash[i].maxsize;
						explosion->flash[i].growing=false;
					}
					explosion->flash[i].base.h=explosion->flash[i].base.w;
					explosion->flash[i].base.x-=increment/2.0f;
					explosion->flash[i].base.y-=increment/2.0f;
				}
				else{
					explosion->flash[i].base.w-=increment;
					if(explosion->flash[i].base.w>0.0f)
						done=false;
					else
						explosion->flash[i].base.w=0.0f;
					explosion->flash[i].base.h=explosion->flash[i].base.w;
					explosion->flash[i].base.x+=increment/2.0f;
					explosion->flash[i].base.y+=increment/2.0f;
				}
			}
			else{
				done=false;
				explosion->flash[i].timer_delay-=state->gamespeed;
			}
		}
		// background cloud
		float increment=((EXPLOSION_FLASH_MAX_GROW_RATE*fabs(explosion->cloud.maxsize-explosion->cloud.base.w))+0.004f)*state->gamespeed;
		if(!state->player.dead&&explosion->sealevel){
			explosion->cloud.base.x+=state->player.xv/GROUP_DEPTH;
			explosion->cloud.base.y+=state->player.yv/GROUP_DEPTH;
		}
		if(explosion->cloud.growing){
			done=false;
			const float GROWTH_MULT=1.8f;
			explosion->cloud.base.w+=increment*GROWTH_MULT;
			if(explosion->cloud.base.w>explosion->cloud.maxsize){
				explosion->cloud.base.w=explosion->cloud.maxsize;
				explosion->cloud.growing=false;
			}
			explosion->cloud.base.h=explosion->cloud.base.w;
			explosion->cloud.base.x-=(increment*GROWTH_MULT)/2.0f;
			explosion->cloud.base.y-=(increment*GROWTH_MULT)/2.0f;
		}
		else{
			const float GROWTH_MULT=0.5f;
			explosion->cloud.base.w-=increment*GROWTH_MULT;
			if(explosion->cloud.base.w>0.0f)
				done=false;
			else
				explosion->cloud.base.w=0.0f;
			explosion->cloud.base.h=explosion->cloud.base.w;
			explosion->cloud.base.x+=(increment*GROWTH_MULT)/2.0f;
			explosion->cloud.base.y+=(increment*GROWTH_MULT)/2.0f;
		}
		if(done){
			explosion=deleteexplosion(state,explosion,prevexplosion);
			continue;
		}
		prevexplosion=explosion;
		explosion=explosion->next;
	}

	// proc player
	if(!state->player.dead){
		state->player.base.x+=state->player.xv*state->gamespeed;
		state->player.base.y+=state->player.yv*state->gamespeed;
		if(state->player.reload>0.0f)state->player.reload-=state->gamespeed;
		if(state->player.timer_smoke<=0.0f){
			newsmoke(state,&state->player.base,0.4f,0.3f,state->player.health/100.0f);
			state->player.timer_smoke=PLAYER_SMOKE;
		}
		else state->player.timer_smoke-=state->gamespeed;
	}
	else if(!--state->gameoverdelay){
		// delete all messages in the queue
		if(state->sounds&&state->player.engine!=NULL){
			stopsound(state->soundengine,state->player.engine);
			state->player.engine=NULL;
		}
		for(struct message *message=state->messagelist;message!=NULL;message=deletemessage(state,message,NULL));
		if(!menu_end(state))
			return false;
		return core(state);
	}

	// proc messages
	if(state->messagelist){
		if(--state->messagelist->ttl==-30){
			deletemessage(state,state->messagelist,NULL);
		}
	}

	// joysticks
	state->joy_top.x=state->joy_base.x+(JOYBASE_SIZE/2.0f)-(JOYTOP_SIZE/2.0f);
	state->joy_top.y=state->joy_base.y+(JOYBASE_SIZE/2.0f)-(JOYTOP_SIZE/2.0f);
	state->fire=false;
	state->bomb=false;
	state->player.xv=-cosf(state->player.base.rot)*PLAYER_SPEED;
	state->player.yv=-sinf(state->player.base.rot)*PLAYER_SPEED;
	align(&state->player.base.rot,PLAYER_TURN_SPEED*state->gamespeed,state->player.targetrot);
	for(int i=0;i<2;++i){
		if(!state->pointer[i].active)
			continue;
		if(state->pointer[i].x>0.0f){
			// fire button
			if(state->pointer[i].x>state->joy_fire.x&&state->pointer[i].x<state->joy_fire.x+JOYFIRE_SIZE&&
					state->pointer[i].y>state->joy_fire.y&&state->pointer[i].y<state->joy_fire.y+JOYFIRE_SIZE){
				state->fire=true;
			}
			else if(state->pointer[i].x>state->joy_bomb.x&&state->pointer[i].x<state->joy_bomb.x+JOYBOMB_SIZE&&
					state->pointer[i].y>state->joy_bomb.y&&state->pointer[i].y<state->joy_bomb.y+JOYBOMB_SIZE){
				state->bomb=true;
			}
		}
		else{
			// joystick
			state->joy_top.x=state->pointer[i].x-(JOYTOP_SIZE/2.0f);
			state->joy_top.y=state->pointer[i].y-(JOYTOP_SIZE/2.0f);
			float angle=atan2f((state->joy_base.y+(JOYBASE_SIZE/2.0f))-(state->joy_top.y+(JOYTOP_SIZE/2.0f)),
					(state->joy_base.x+(JOYBASE_SIZE/2.0f))-(state->joy_top.x+(JOYTOP_SIZE/2.0f)));
			state->player.targetrot=angle;

			if(distance(state->joy_top.x+(JOYTOP_SIZE/2.0f),state->joy_base.x+(JOYBASE_SIZE/2.0f),
						state->joy_top.y+(JOYTOP_SIZE/2.0f),state->joy_base.y+(JOYBASE_SIZE/2.0f))>JOYTOP_DIST){
				state->joy_top.x=(state->joy_base.x+(JOYBASE_SIZE/2.0f)-(JOYTOP_SIZE/2.0f))-cosf(angle)*JOYTOP_DIST;
				state->joy_top.y=(state->joy_base.y+(JOYBASE_SIZE/2.0f)-(JOYTOP_SIZE/2.0f))-sinf(angle)*JOYTOP_DIST;
			}
		}
	}

	// radar blips
	state->radarblink+=0.06f;
	if(state->radarblink>1000.0f)
		state->radarblink=0.0f;
	
	// pause menu
	if(state->back){
		state->back=false;
		if(!menu_pause(state))
			return false;
	}

	return true;
}

void render(struct state *state){
	glClear(GL_COLOR_BUFFER_BIT);
	/*glBindTexture(GL_TEXTURE_2D,state->assets.texture[TID_BACKGROUND].object);
	draw(state,&state->background);*/

	// render groups
	if(state->grouplist){
		glBindTexture(GL_TEXTURE_2D,state->assets.texture[TID_GROUP].object);
		glUniform4f(state->uniform.rgba,0.8f,0.8f,1.0f,1.0f);
		for(struct group *group=state->grouplist;group!=NULL;group=group->next){
			if(group->dead)
				glUniform4f(state->uniform.rgba,0.4f,0.4f,0.5f,1.0f);
			else
				glUniform4f(state->uniform.rgba,0.8f,0.8f,0.8f,1.0f);
			draw(state,&group->base);
		}
	}
	glUniform4f(state->uniform.rgba,1.0f,1.0f,1.0f,1.0f);

	// render bombs
	if(state->bomblist){
		glBindTexture(GL_TEXTURE_2D,state->assets.texture[TID_BOMB].object);
		for(struct bomb *bomb=state->bomblist;bomb!=NULL;bomb=bomb->next)
			draw(state,&bomb->base);
	}

	// render clouds
	if(state->cloudlist){
		glBindTexture(GL_TEXTURE_2D,state->assets.texture[TID_CLOUD].object);
		for(struct cloud *cloud=state->cloudlist;cloud!=NULL;cloud=cloud->next){
			glUniform4f(state->uniform.rgba,1.0f,1.0f,1.0f,cloud->alpha);
			draw(state,&cloud->base);
		}
	}

	// render explosions
	if(state->explosionlist){
		glBindTexture(GL_TEXTURE_2D,state->assets.texture[TID_FLASH].object);
		for(struct explosion *explosion=state->explosionlist;explosion!=NULL;explosion=explosion->next){
			// draw background cloud
			glUniform4f(state->uniform.rgba,1.0f,1.0f,1.0f,1.0f);
			draw(state,&explosion->cloud.base);

			for(int i=0;i<EXPLOSION_FLASH_COUNT;++i){
				glUniform4f(state->uniform.rgba,explosion->flash[i].rgb[0],explosion->flash[i].rgb[1],explosion->flash[i].rgb[2],1.0f);
				draw(state,&explosion->flash[i].base);
			}
		}
	}

	// render health
	if(state->healthlist){
		glUniform4f(state->uniform.rgba,1.0f,1.0f,1.0f,1.0f);
		glBindTexture(GL_TEXTURE_2D,state->assets.texture[TID_HEALTH].object);
		for(struct health *health=state->healthlist;health!=NULL;health=health->next)
			draw(state,&health->base);
		if(!state->player.dead){
			// render indicators
			glBindTexture(GL_TEXTURE_2D,state->assets.texture[TID_HEALTHINDICATOR].object);
			struct base i;
			i.count=1.0f;
			i.frame=0.0f;
			i.w=INDICATOR_WIDTH;
			i.h=INDICATOR_HEIGHT;
			for(struct health *health=state->healthlist;health!=NULL;health=health->next){
				if(health->base.x+HEALTH_SIZE<PLAYER_LEFT_BOUNDARY||health->base.x>PLAYER_RIGHT_BOUNDARY||
						health->base.y+HEALTH_SIZE<PLAYER_TOP_BOUNDARY||health->base.y>PLAYER_BOTTOM_BOUNDARY){
					float x=health->base.x+(HEALTH_SIZE/2.0f)-(INDICATOR_WIDTH/2.0f);
					float y=health->base.y+(HEALTH_SIZE/2.0f)-(INDICATOR_HEIGHT/2.0f);
					if(health->base.x+HEALTH_SIZE<PLAYER_LEFT_BOUNDARY)x=PLAYER_LEFT_BOUNDARY;
					else if(health->base.x>PLAYER_RIGHT_BOUNDARY)x=PLAYER_RIGHT_BOUNDARY-INDICATOR_WIDTH;
					if(health->base.y+HEALTH_SIZE<PLAYER_TOP_BOUNDARY)y=PLAYER_TOP_BOUNDARY+(INDICATOR_WIDTH-INDICATOR_HEIGHT);
					else if(health->base.y>PLAYER_BOTTOM_BOUNDARY)y=PLAYER_BOTTOM_BOUNDARY-INDICATOR_WIDTH;
					float angle=atan2f((y+(INDICATOR_HEIGHT/2.0f))-(health->base.y+(HEALTH_SIZE/2.0f)),
							(x+(INDICATOR_WIDTH/2.0f))-(health->base.x+(HEALTH_SIZE/2.0f)));
					i.x=x;
					i.y=y;
					i.rot=angle;
					draw(state,&i);
				}
			}
		}
	}

	// render smoke
	if(state->smokelist){
		glBindTexture(GL_TEXTURE_2D,state->assets.texture[TID_SMOKE].object);
		for(struct smoke *smoke=state->smokelist;smoke!=NULL;smoke=smoke->next){
			glUniform4f(state->uniform.rgba,smoke->gray,smoke->gray,smoke->gray,smoke->alpha);
			draw(state,&smoke->base);
		}
		glUniform4f(state->uniform.rgba,1.0f,1.0f,1.0f,1.0f);
	}

	// render missiles
	if(state->missilelist){
		glBindTexture(GL_TEXTURE_2D,state->assets.texture[TID_MISSILE].object);
		for(struct missile *missile=state->missilelist;missile!=NULL;missile=missile->next)
			draw(state,&missile->base);

		// indicators
		if(!state->player.dead){
			glBindTexture(GL_TEXTURE_2D,state->assets.texture[TID_INDICATOR].object);
			for(struct missile *missile=state->missilelist;missile!=NULL;missile=missile->next){
				if(missile->base.x+MISSILE_WIDTH<PLAYER_LEFT_BOUNDARY||missile->base.x>PLAYER_RIGHT_BOUNDARY||
						missile->base.y+MISSILE_HEIGHT<PLAYER_TOP_BOUNDARY||missile->base.y>PLAYER_BOTTOM_BOUNDARY){
					float x=missile->base.x+(MISSILE_WIDTH/2.0f)-(INDICATOR_WIDTH/2.0f);
					float y=missile->base.y+(MISSILE_HEIGHT/2.0f)-(INDICATOR_HEIGHT/2.0f);
					if(missile->base.x+MISSILE_WIDTH<PLAYER_LEFT_BOUNDARY)x=PLAYER_LEFT_BOUNDARY;
					else if(missile->base.x>PLAYER_RIGHT_BOUNDARY)x=PLAYER_RIGHT_BOUNDARY-INDICATOR_WIDTH;
					if(missile->base.y+MISSILE_HEIGHT<PLAYER_TOP_BOUNDARY)y=PLAYER_TOP_BOUNDARY+(INDICATOR_WIDTH-INDICATOR_HEIGHT);
					else if(missile->base.y>PLAYER_BOTTOM_BOUNDARY)y=PLAYER_BOTTOM_BOUNDARY-INDICATOR_WIDTH;
					float angle=atan2f((y+(INDICATOR_HEIGHT/2.0f))-(missile->base.y+(MISSILE_HEIGHT/2.0f)),
							(x+(INDICATOR_WIDTH/2.0f))-(missile->base.x+(MISSILE_WIDTH/2.0f)));
					struct base indicator={x,y,INDICATOR_WIDTH,INDICATOR_HEIGHT,angle,1.0f,0.0f};
					draw(state,&indicator);
				}
			}
		}
	}
	
	// render bullets
	if(state->bulletlist){
		glBindTexture(GL_TEXTURE_2D,state->assets.texture[TID_BULLET].object);
		for(struct bullet *bullet=state->bulletlist;bullet!=NULL;bullet=bullet->next)
			draw(state,&bullet->base);
	}

	// render player
	if(!state->player.dead){
		glBindTexture(GL_TEXTURE_2D,state->assets.texture[TID_PLAYER].object);
		draw(state,&state->player.base);
	}

	// render enemies
	if(state->enemylist){
		glBindTexture(GL_TEXTURE_2D,state->assets.texture[TID_ENEMY].object);
		for(struct enemy *enemy=state->enemylist;enemy!=NULL;enemy=enemy->next)
			draw(state,&enemy->base);
	}
	
	// render controls
	if(!state->player.dead){
		// fire button
		if(state->fire)
			glUniform4f(state->uniform.rgba,0.6f,0.6f,0.6f,1.0f);
		else
			glUniform4f(state->uniform.rgba,1.0f,1.0f,1.0f,1.0f);
		glBindTexture(GL_TEXTURE_2D,state->uiassets.texture[TID_JOYFIRE].object);
		uidraw(state,&state->joy_fire);
		// bomb button
		if(state->bomb)
			glUniform4f(state->uniform.rgba,0.6f,0.6f,0.6f,0.6f);
		else
			glUniform4f(state->uniform.rgba,1.0f,1.0f,1.0f,0.3f);
		glBindTexture(GL_TEXTURE_2D,state->uiassets.texture[TID_JOYBOMB].object);
		uidraw(state,&state->joy_bomb);
		glUniform4f(state->uniform.texcoords,0.0f,(BOMB_RECHARGE-state->player.timer_bomb)/BOMB_RECHARGE,0.0f,1.0f);
		glUniform2f(state->uniform.size,state->joy_bomb.w*((BOMB_RECHARGE-state->player.timer_bomb)/BOMB_RECHARGE),state->joy_bomb.h);
		glUniform4f(state->uniform.rgba,1.0f,0.0f,0.0f,0.6f);
		glDrawArrays(GL_TRIANGLE_STRIP,0,4);
		// radar
		float blink=(sinf(state->radarblink)+1.0f)/2.0f;
		glUniform4f(state->uniform.rgba,0.6f,0.05f,0.05f,blink);
		glBindTexture(GL_TEXTURE_2D,state->uiassets.texture[TID_BLOB].object);
		for(struct group *group=state->grouplist;group!=NULL;group=group->next){
			if(group->dead)
				continue;
			float angle=atan2f((state->player.base.y+(PLAYER_HEIGHT/2.0f))-(group->base.y+(GROUP_HEIGHT/2.0f)),
					(state->player.base.x+(PLAYER_WIDTH/2.0f))-(group->base.x+(GROUP_WIDTH/2.0f)));
			float dist=distance(state->player.base.x+(PLAYER_WIDTH/2.0f),group->base.x+(GROUP_WIDTH/2.0f),
					state->player.base.y+(PLAYER_HEIGHT/2.0f),group->base.y+(GROUP_HEIGHT/2.0f))/40.0f;
			dist*=GROUP_DEPTH;
			if(dist>0.6f)
				dist=0.6f;
			float x,y;
			x=state->joy_fire.x+(JOYFIRE_SIZE/2.0f)-(cosf(angle)*dist);
			y=state->joy_fire.y+(JOYFIRE_SIZE/2.0f)-(sinf(angle)*dist);
			struct base blob={x,y,0.1f,0.1f,0.0f,1.0f,0.0f};
			uidraw(state,&blob);
		}
		glUniform4f(state->uniform.rgba,0.6f,0.05f,0.05f,blink);
		for(struct enemy *enemy=state->enemylist;enemy!=NULL;enemy=enemy->next){
			float angle=atan2f((state->player.base.y+(PLAYER_HEIGHT/2.0f))-(enemy->base.y+(ENEMY_HEIGHT/2.0f)),
					(state->player.base.x+(PLAYER_WIDTH/2.0f))-(enemy->base.x+(ENEMY_WIDTH/2.0f)));
			float dist=distance(state->player.base.x+(PLAYER_WIDTH/2.0f),enemy->base.x+(ENEMY_WIDTH/2.0f),
					state->player.base.y+(PLAYER_HEIGHT/2.0f),enemy->base.y+(ENEMY_HEIGHT/2.0f))/40.0f;
			if(dist>0.6f)
				dist=0.6f;
			float x,y;
			x=state->joy_fire.x+(JOYFIRE_SIZE/2.0f)-(cosf(angle)*dist);
			y=state->joy_fire.y+(JOYFIRE_SIZE/2.0f)-(sinf(angle)*dist);
			struct base blob={x,y,0.05f,0.05f,0.0f,1.0f,0.0f};
			uidraw(state,&blob);
		}
		// joysticks
		glUniform4f(state->uniform.rgba,1.0f,1.0f,1.0f,1.0f);
		glBindTexture(GL_TEXTURE_2D,state->uiassets.texture[TID_JOYBASE].object);
		uidraw(state,&state->joy_base);
		glBindTexture(GL_TEXTURE_2D,state->uiassets.texture[TID_JOYTOP].object);
		uidraw(state,&state->joy_top);
	}
	else glUniform1f(state->uniform.rot,0.0f);

	// render messages
	glBindTexture(GL_TEXTURE_2D,state->font.main->atlas);
	if(state->messagelist&&state->messagelist->ttl>0){
		float alpha;
		float y;
		// fading out (ttl approaching zero)
		if(state->messagelist->ttl<MESSAGE_PINCH){
			alpha=(float)state->messagelist->ttl/MESSAGE_PINCH;
			y=MESSAGE_Y+(0.3f/state->messagelist->ttl);
		}
		// fading in
		else if(MESSAGE_TTL-state->messagelist->ttl<MESSAGE_PINCH){
			alpha=((float)MESSAGE_TTL-state->messagelist->ttl)/MESSAGE_PINCH;
			y=MESSAGE_Y-(0.3f/(MESSAGE_TTL-state->messagelist->ttl));
		}
		else{
			alpha=1.0f;
			y=MESSAGE_Y;
		}
		glUniform4f(state->uniform.rgba,1.0f,1.0f,0.8f,alpha);
		drawtextcentered(state->font.main,0.0f,y,state->messagelist->text);
	}
	glUniform4f(state->uniform.rgba,1.0f,1.0f,1.0f,1.0f);

	if(!state->player.dead){
		char pointsmessage[20];
		sprintf(pointsmessage,"%d",(int)state->points);
		drawtextcentered(state->font.main,0.0f,state->rect.bottom-state->font.main->fontsize-0.1f,pointsmessage);
	}
	
	// fps counter
	{
		static char statustext[50];
		static int lasttime,fps;
		int currenttime=time(NULL);
		if(currenttime!=lasttime){
			lasttime=currenttime;
			sprintf(statustext,"[ace] %d fps",fps);
			fps=0;
		}
		else ++fps;
		glUniform4f(state->uniform.rgba,0.0f,0.0f,0.0f,1.0f);
		glBindTexture(GL_TEXTURE_2D,state->font.main->atlas);
		drawtext(state->font.main,state->rect.left+0.05f,state->rect.top+0.05f,statustext);
	}
}

void init(struct state *state){
	load_settings(state);
	load_stats(state);

	state->back=false;
	memset(state->pointer,0,sizeof(struct crosshair)*2);
	state->showmenu=true;

	state->rect.left=-8.0f;
	state->rect.right=8.0f;
	state->rect.bottom=4.5f;
	state->rect.top=-4.5f;

	state->background.x=state->rect.left;
	state->background.y=state->rect.top;
	state->background.w=state->rect.right*2.0f;
	state->background.h=state->rect.bottom*2.0f;
	state->background.rot=0.0f;

	state->joy_base.w=JOYBASE_SIZE;
	state->joy_base.h=JOYBASE_SIZE;
	state->joy_base.x=-7.0f;
	state->joy_base.y=1.75f;
	state->joy_base.rot=0.0f;
	state->joy_top.w=JOYTOP_SIZE;
	state->joy_top.h=JOYTOP_SIZE;
	state->joy_top.x=state->joy_base.x+(JOYBASE_SIZE/2.0f)-(JOYTOP_SIZE/2.0f);
	state->joy_top.y=state->joy_base.y+(JOYBASE_SIZE/2.0f)-(JOYTOP_SIZE/2.0f);
	state->joy_top.rot=0.0f;
	state->joy_fire.x=5.5f;
	state->joy_fire.y=2.1f;
	state->joy_fire.rot=0.0f;
	state->joy_fire.w=JOYFIRE_SIZE;
	state->joy_fire.h=JOYFIRE_SIZE;
	state->joy_bomb.w=JOYBOMB_SIZE;
	state->joy_bomb.h=JOYBOMB_SIZE;
	state->joy_bomb.x=7.15f;
	state->joy_bomb.y=3.65f;
	state->joy_bomb.rot=0.0f;
	state->joy_bomb.count=1.0f;
	state->joy_bomb.frame=0.0f;

	state->player.engine=NULL;
	state->player.base.w=PLAYER_WIDTH;
	state->player.base.h=PLAYER_HEIGHT;
	state->player.base.count=1.0f;

	state->cloudlist=NULL;
	state->largecloudlist=NULL;
	state->enemylist=NULL;
	state->grouplist=NULL;
	state->bomblist=NULL;
	state->missilelist=NULL;
	state->bulletlist=NULL;
	state->smokelist=NULL;
	state->healthlist=NULL;
	state->explosionlist=NULL;
	state->messagelist=NULL;
}

void reset(struct state *state){
	for(struct cloud *cloud=state->cloudlist;cloud!=NULL;cloud=deletecloud(state,cloud,NULL));
	state->cloudlist=NULL;
	for(struct largecloud *largecloud=state->largecloudlist;largecloud!=NULL;largecloud=deletelargecloud(state,largecloud,NULL));
	state->largecloudlist=NULL;
	for(struct missile *missile=state->missilelist;missile!=NULL;missile=deletemissile(state,missile,NULL));
	state->missilelist=NULL;
	for(struct bullet *bullet=state->bulletlist;bullet!=NULL;bullet=deletebullet(state,bullet,NULL));
	state->bulletlist=NULL;
	for(struct smoke *smoke=state->smokelist;smoke!=NULL;smoke=deletesmoke(state,smoke,NULL));
	state->smokelist=NULL;
	for(struct health *health=state->healthlist;health!=NULL;health=deletehealth(state,health,NULL));
	state->healthlist=NULL;
	for(struct enemy *enemy=state->enemylist;enemy!=NULL;enemy=deleteenemy(state,enemy,NULL));
	state->enemylist=NULL;
	for(struct group *group=state->grouplist;group!=NULL;group=deletegroup(state,group,NULL));
	state->grouplist=NULL;
	for(struct bomb *bomb=state->bomblist;bomb!=NULL;bomb=deletebomb(state,bomb,NULL));
	state->bomblist=NULL;
	for(struct explosion *explosion=state->explosionlist;explosion!=NULL;explosion=deleteexplosion(state,explosion,NULL));
	state->explosionlist=NULL;
	for(struct message *message=state->messagelist;message!=NULL;message=deletemessage(state,message,NULL));
	state->messagelist=NULL;
	
	state->focused_enemy=NULL;
	state->gamespeed=1.0f;
	state->fire=false;
	state->bomb=false;
	state->gameoverdelay=GAMEOVER_DELAY;
	state->radarblink=0.0f;
	state->points=0.0f;
	state->player.health=100;
	state->player.base.x=-PLAYER_WIDTH/2.0f;
	state->player.base.y=-PLAYER_HEIGHT/2.0f;
	state->player.xv=0.0f;
	state->player.yv=PLAYER_SPEED;
	state->player.targetrot=0.0f;
	state->player.base.rot=0.0f;
	state->player.reload=0;
	state->player.base.frame=0;
	state->player.timer_smoke=0;
	state->player.timer_bomb=0.0f;
	state->player.bombs=0;
	state->player.dead=false;
	state->player.victories=0;
}

