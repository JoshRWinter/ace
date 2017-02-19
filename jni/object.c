#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <android_native_app_glue.h>
#include <stdlib.h>
#include <strings.h>
#include <math.h>
#include "defs.h"

void newenemy(struct state *state){
	int count=0;
	for(struct enemy *enemy=state->enemylist;enemy!=NULL;enemy=enemy->next)
		++count;
	if(count>2)
		return;

	struct enemy *enemy=malloc(sizeof(struct enemy));
	enemy->base.w=ENEMY_WIDTH;
	enemy->base.h=ENEMY_HEIGHT;
	const float ENEMY_BOUNDS=10.0f;
	enemy->base.x=state->player.base.x+(onein(2)?ENEMY_BOUNDS:-ENEMY_BOUNDS);
	enemy->base.y=state->player.base.y+(onein(2)?ENEMY_BOUNDS:-ENEMY_BOUNDS);
	enemy->base.rot=randomint(1,360)*(M_PI/180.0f);
	enemy->base.frame=0.0f;
	enemy->base.count=2.0f;
	enemy->target.w=ENEMY_WIDTH;
	enemy->target.h=ENEMY_HEIGHT;
	enemy->target.x=enemy->base.x;
	enemy->target.y=enemy->base.y;
	enemy->target.rot=0.0f;
	enemy->dead=false;
	enemy->dying=0.0f;
	enemy->health=100;
	enemy->timer_smoke=0;
	enemy->timer_reload=0;
	enemy->timer_frame=0.0f;
	enemy->next=state->enemylist;
	state->enemylist=enemy;
}
struct enemy *deleteenemy(struct state *state,struct enemy *enemy,struct enemy *prev){
	if(enemy==state->focused_enemy)state->focused_enemy=NULL;
	if(prev!=NULL)prev->next=enemy->next;
	else state->enemylist=enemy->next;
	void *temp=enemy->next;
	free(enemy);
	return temp;
}

void newgroup(struct state *state){
	float xoff=(randomint(125,170)/10.0f)*(randomint(0,1)?1.0f:-1.0f);
	float yoff=(randomint(125,170)/10.0f)*(randomint(0,1)?1.0f:-1.0f);
	struct group *group=malloc(sizeof(struct group));
	group->base.w=GROUP_WIDTH;
	group->base.h=GROUP_HEIGHT;
	group->base.x=state->player.base.x+xoff;
	group->base.y=state->player.base.y+yoff;
	group->base.rot=randomint(1,360)*(M_PI/180.0f);
	group->base.count=1.0f;
	group->base.frame=0.0f;
	group->health=100;
	group->dead=false;
	group->timer_explosions=0.0f;
	group->next=state->grouplist;
	state->grouplist=group;
}
struct group *deletegroup(struct state *state,struct group *group,struct group *prev){
	if(prev!=NULL)prev->next=group->next;
	else state->grouplist=group->next;
	void *temp=group->next;
	free(group);
	return temp;
}

void newbomb(struct state *state){
	struct bomb *bomb=malloc(sizeof(struct bomb));
	bomb->base.w=BOMB_WIDTH;
	bomb->base.h=BOMB_HEIGHT;
	float xoff=randomint(-5,5)/100.0f;
	float yoff=randomint(-5,5)/100.0f;
	bomb->base.x=state->player.base.x+(PLAYER_WIDTH/2.0f)-(BOMB_WIDTH/2.0f)+xoff;
	bomb->base.y=state->player.base.y+(PLAYER_HEIGHT/2.0f)-(BOMB_HEIGHT/2.0f)+yoff;
	bomb->base.rot=state->player.base.rot;
	bomb->base.count=1.0f;
	bomb->base.frame=0.0f;
	bomb->altitude=BOMB_ALTITUDE;
	bomb->depth=2.3f;
	bomb->xv=state->player.xv/2.0f;
	bomb->yv=state->player.yv/2.0f;
	bomb->next=state->bomblist;
	state->bomblist=bomb;
}
struct bomb *deletebomb(struct state *state,struct bomb *bomb,struct bomb *prev){
	if(prev!=NULL)prev->next=bomb->next;
	else state->bomblist=bomb->next;
	void *temp=bomb->next;
	free(bomb);
	return temp;
}

void newmissile(struct state *state,struct enemy *enemy){
	int count=0;
	for(struct missile *missile=state->missilelist;missile!=NULL;missile=missile->next)
		++count;
	if(count>1)
		return;
	struct missile *missile=malloc(sizeof(struct missile));
	missile->base.w=MISSILE_WIDTH;
	missile->base.h=MISSILE_HEIGHT;
	missile->base.x=enemy->base.x+(ENEMY_WIDTH/2.0f)-(MISSILE_WIDTH/2.0f);
	missile->base.y=enemy->base.y+(ENEMY_HEIGHT/2.0f)-(MISSILE_HEIGHT/2.0f);
	missile->base.rot=enemy->base.rot;
	missile->base.count=1.0f;
	missile->base.frame=0.0f;
	missile->timer_smoke=0.0f;
	missile->dead=false;
	missile->ttl=MISSILE_TTL;
	missile->sway=0.0f;
	missile->next=state->missilelist;
	state->missilelist=missile;
}
struct missile *deletemissile(struct state *state,struct missile *missile,struct missile *prev){
	if(prev!=NULL)prev->next=missile->next;
	else state->missilelist=missile->next;
	void *temp=missile->next;
	free(missile);
	return temp;
}

void newbullet(struct state *state,struct base *owner){
	const float DISPLACE=0.2f;
	const float ANGLE_OFFSET=0.9f;

	struct bullet *b1=malloc(sizeof(struct bullet));
	struct bullet *b2;
	if(owner==&state->player.base)
		b2=malloc(sizeof(struct bullet));

	b1->base.w=BULLET_WIDTH;
	b1->base.h=BULLET_HEIGHT;
	if(owner==&state->player.base){
		b1->base.x=(owner->x+(owner->w/2.0f)-(BULLET_WIDTH/2.0f))-(cosf(owner->rot+ANGLE_OFFSET)*DISPLACE);
		b1->base.y=(owner->y+(owner->h/2.0f)-(BULLET_HEIGHT/2.0f))-(sinf(owner->rot+ANGLE_OFFSET)*DISPLACE);
	}
	else{
		b1->base.x=owner->x+(ENEMY_WIDTH/2.0f)-(BULLET_WIDTH/2.0f);
		b1->base.y=owner->y+(ENEMY_HEIGHT/2.0f)-(BULLET_HEIGHT/2.0f);
	}
	b1->base.rot=owner->rot;
	b1->base.frame=0.0f;
	b1->base.count=1.0f;
	b1->xv=-cosf(owner->rot)*BULLET_SPEED;
	b1->yv=-sinf(owner->rot)*BULLET_SPEED;
	b1->owner=owner;
	b1->next=state->bulletlist;
	state->bulletlist=b1;

	if(owner==&state->player.base){
		b2->base.w=BULLET_WIDTH;
		b2->base.h=BULLET_HEIGHT;
		b2->base.x=(owner->x+(owner->w/2.0f)-(BULLET_WIDTH/2.0f))-(cosf(owner->rot-ANGLE_OFFSET)*DISPLACE);
		b2->base.y=(owner->y+(owner->h/2.0f)-(BULLET_HEIGHT/2.0f))-(sinf(owner->rot-ANGLE_OFFSET)*DISPLACE);
		b2->base.rot=owner->rot;
		b2->base.frame=0.0f;
		b2->base.count=1.0f;
		b2->xv=-cosf(owner->rot)*BULLET_SPEED;
		b2->yv=-sinf(owner->rot)*BULLET_SPEED;
		b2->owner=owner;
		b2->next=state->bulletlist;
		state->bulletlist=b2;
	}
}
struct bullet *deletebullet(struct state *state,struct bullet *bullet,struct bullet *prev){
	if(prev!=NULL)prev->next=bullet->next;
	else state->bulletlist=bullet->next;
	void *temp=bullet->next;
	free(bullet);
	return temp;
}

void newsmoke(struct state *state,struct base *base,float size,float alpha,float gray){
	const float SMOKE_SPEED=0.01f;
	struct smoke *smoke=malloc(sizeof(struct smoke));
	smoke->base.w=size;
	smoke->base.h=size;
	smoke->base.x=base->x+(base->w/2.0f)-(size/2.0f);
	smoke->base.y=base->y+(base->h/2.0f)-(size/2.0f);
	smoke->base.rot=randomint(1,360)*(M_PI/180.0f);
	smoke->base.frame=0.0f;
	smoke->base.count=1.0f;
	smoke->alpha=alpha;
	smoke->gray=gray;
	smoke->xv=cosf(base->rot)*SMOKE_SPEED;
	smoke->yv=sinf(base->rot)*SMOKE_SPEED;
	smoke->next=state->smokelist;
	state->smokelist=smoke;
}
struct smoke *deletesmoke(struct state *state,struct smoke *smoke,struct smoke *prev){
	if(prev!=NULL)prev->next=smoke->next;
	else state->smokelist=smoke->next;
	void *temp=smoke->next;
	free(smoke);
	return temp;
}

void newhealth(struct state *state,struct enemy *enemy){
	struct health *health=malloc(sizeof(struct health));
	health->base.w=HEALTH_SIZE;
	health->base.h=HEALTH_SIZE;
	health->base.x=enemy->base.x+(ENEMY_WIDTH/2.0f)-(HEALTH_SIZE/2.0f);
	health->base.y=enemy->base.y+(ENEMY_HEIGHT/2.0f)-(HEALTH_SIZE/2.0f);
	health->xv=enemy->xv;
	health->yv=enemy->yv;
	health->base.rot=0.0f;
	health->base.count=1.0f;
	health->base.frame=0.0f;
	health->next=state->healthlist;
	state->healthlist=health;
}
struct health *deletehealth(struct state *state,struct health *health,struct health *prev){
	if(prev!=NULL)prev->next=health->next;
	else state->healthlist=health->next;
	void *temp=health->next;
	free(health);
	return temp;
}

void newcloud(struct state *state){
	int count=0;
	for(struct cloud *cloud=state->cloudlist;cloud!=NULL;cloud=cloud->next)
		++count;
	if(count>7)
		return;

	struct cloud *cloud=malloc(sizeof(struct cloud));
	cloud->base.w=randomint(150.0f,CLOUD_SIZE*100.0f)/100.0f;
	cloud->base.h=cloud->base.w;
	cloud->base.rot=0.0f;
	cloud->base.frame=0.0f;
	cloud->base.count=1.0f;
	if(cloud->base.w>1.75)
		cloud->alpha=randomint(45,60)/100.0f;
	else
		cloud->alpha=randomint(50,65)/100.0f;
	cloud->parallax=((randomint(5,15)/100.0f)+(cloud->base.w-2.0f))/2.25f;//randomint(0,20)/100.0f;
	int loops=0;
	while(1){
		// scuttle the clouds offscreen
		// weird, but a reliable way of getting a good distributed cluster of clouds
		float xoffset,yoffset;
		float angle=randomint(1,360)*(M_PI/180.0f);
		const float disperse=randomint(10,25);
		float xv=cosf(angle)*disperse;
		float yv=sinf(angle)*disperse;
		xoffset=state->player.base.x+xv;
		yoffset=state->player.base.y+yv;
		cloud->base.x=xoffset;
		cloud->base.y=yoffset;
		int nocollide=true;
		for(struct cloud *c=state->cloudlist;c!=NULL;c=c->next){
			if(collide(&c->base,&cloud->base,-0.5f)){
				nocollide=false;
				break;
			}
		}
		if(nocollide)
			break;
		if(++loops>5)
			break;
	}
	cloud->next=state->cloudlist;
	state->cloudlist=cloud;
}
struct cloud *deletecloud(struct state *state,struct cloud *cloud,struct cloud *prev){
	if(prev!=NULL)prev->next=cloud->next;
	else state->cloudlist=cloud->next;
	void *temp=cloud->next;
	free(cloud);
	return temp;
}

void newlargecloud(struct state *state,int top){
	struct largecloud *cloud=malloc(sizeof(struct largecloud));
	cloud->base.w=randomint((LARGECLOUD_SIZE-2.5f)*10.0f,LARGECLOUD_SIZE*10.0f)/10.0f;
	cloud->base.h=cloud->base.w;
	cloud->base.x=state->rect.left-LARGECLOUD_SIZE;
	cloud->base.y=randomint((state->rect.top-(LARGECLOUD_SIZE/2.0f))*10.0f,(state->rect.bottom+(LARGECLOUD_SIZE/2.0f))*10.0f)/10.0f;
	cloud->yoffset=cloud->base.y;
	cloud->base.rot=0.0f;
	cloud->base.count=1.0f;
	cloud->base.frame=0.0f;
	cloud->top=top;
	cloud->alpha=1.0f;
	cloud->xv=cloud->base.w/50.0f;
	cloud->yv=0.0f;
	cloud->next=state->largecloudlist;
	state->largecloudlist=cloud;
}
struct largecloud *deletelargecloud(struct state *state,struct largecloud *cloud,struct largecloud *prev){
	if(prev!=NULL)prev->next=cloud->next;
	else state->largecloudlist=cloud->next;
	void *temp=cloud->next;
	free(cloud);
	return temp;
}

void newexplosion(struct state *state,float x,float y,float size,int sealevel){
	// don't generate explosion if it would be offscreen
	float xdiff=fabs(x-(state->player.base.x+(PLAYER_WIDTH/2.0f)));
	if(xdiff>state->rect.right+1.0f)
		return;
	float ydiff=fabs(y-(state->player.base.y+(PLAYER_HEIGHT/2.0f)));
	if(ydiff>state->rect.bottom+1.0f)
		return;
	struct explosion *explosion=malloc(sizeof(struct explosion));
	explosion->sealevel=sealevel;
	for(int i=0;i<EXPLOSION_FLASH_COUNT;++i){
		const float SIZE_MULT=0.5f;
		explosion->flash[i].base.x=randomint((x-(size*SIZE_MULT))*10.0f,(x+size)*10.0f)/10.0f;
		explosion->flash[i].base.y=randomint((y-(size*SIZE_MULT))*10.0f,(y+size)*10.0f)/10.0f;
		explosion->flash[i].base.w=0.0f;
		explosion->flash[i].base.h=0.0f;
		explosion->flash[i].base.rot=0.0f;
		explosion->flash[i].base.frame=0.0f;
		explosion->flash[i].base.count=2.0f;
		explosion->flash[i].maxsize=randomint((size*EXPLOSION_FLASH_SIZE_MIN_MULTIPLIER)*10.0f,(size*EXPLOSION_FLASH_SIZE_MAX_MULTIPLIER)*10.0f)/10.0f;
		explosion->flash[i].growing=true;
		explosion->flash[i].timer_delay=randomint(EXPLOSION_FLASH_MIN_TIMER,EXPLOSION_FLASH_MAX_TIMER);
		int color=randomint(0,2);
		switch(color){
			case 0:
				explosion->flash[i].rgb[0]=1.0f;
				explosion->flash[i].rgb[1]=0.0f;
				explosion->flash[i].rgb[2]=0.0f;
				break;
			case 1:
				explosion->flash[i].rgb[0]=1.0f;
				explosion->flash[i].rgb[1]=1.0f;
				explosion->flash[i].rgb[2]=0.0f;
				break;
			case 2:
				explosion->flash[i].rgb[0]=1.0f;
				explosion->flash[i].rgb[1]=0.6f;
				explosion->flash[i].rgb[2]=0.1f;
				break;
		}
	}

	// background cloud
	explosion->cloud.base.w=0.0f;
	explosion->cloud.base.h=0.0f;
	explosion->cloud.base.x=x;
	explosion->cloud.base.y=y;
	explosion->cloud.base.rot=randomint(1,360)*(M_PI/180.0f);
	explosion->cloud.base.count=2.0f;
	explosion->cloud.base.frame=1.0f;
	explosion->cloud.rgb[0]=1.0f;
	explosion->cloud.rgb[1]=1.0f;
	explosion->cloud.rgb[2]=1.0f;
	explosion->cloud.maxsize=size*4.1f;
	explosion->cloud.growing=true;
	explosion->cloud.timer_delay=0;

	// explosions are best inserted at the back of the list
	explosion->next=NULL;
	if(state->explosionlist==NULL){
		state->explosionlist=explosion;
	}
	else{
		for(struct explosion *e=state->explosionlist;;e=e->next){
			if(e->next==NULL){
				e->next=explosion;
				break;
			}
		}
	}
}
struct explosion *deleteexplosion(struct state *state,struct explosion *explosion,struct explosion *prev){
	if(prev!=NULL)prev->next=explosion->next;
	else state->explosionlist=explosion->next;
	void *temp=explosion->next;
	free(explosion);
	return temp;
}

void newmessage(struct state *state,char *msg){
	if(state->sounds)
		playsound(state->soundengine,state->aassets.sound+SID_MESSAGE,false);
	struct message *message=malloc(sizeof(struct message));
	message->ttl=MESSAGE_TTL;
	int len=strlen(msg);
	int i;
	for(i=0;i<len&&i<MESSAGE_MAX;++i)
		message->text[i]=msg[i];
	message->text[i]=0;
	// messages have to get inserted at the end of the list
	message->next=NULL;
	struct message *m=state->messagelist;
	if(m==NULL)
		state->messagelist=message;
	else{
		while(m->next!=NULL)
			m=m->next;
		m->next=message;
	}
}
struct message *deletemessage(struct state *state,struct message *message,struct message *prev){
	if(prev!=NULL)prev->next=message->next;
	else state->messagelist=message->next;
	void *temp=message->next;
	free(message);
	return temp;
}

