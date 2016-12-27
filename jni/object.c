#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <android_native_app_glue.h>
#include <stdlib.h>
#include <math.h>
#include "defs.h"

void newbullet(struct state *state,struct base *owner){
	const float DISPLACE=0.2f;
	const float ANGLE_OFFSET=0.9f;

	struct bullet *b1=malloc(sizeof(struct bullet));
	struct bullet *b2=malloc(sizeof(struct bullet));

	b1->base.w=BULLET_WIDTH;
	b1->base.h=BULLET_HEIGHT;
	b1->base.x=(owner->x+(owner->w/2.0f)-(BULLET_WIDTH/2.0f))-(cosf(owner->rot+ANGLE_OFFSET)*DISPLACE);
	b1->base.y=(owner->y+(owner->h/2.0f)-(BULLET_HEIGHT/2.0f))-(sinf(owner->rot+ANGLE_OFFSET)*DISPLACE);
	b1->base.rot=owner->rot;
	b1->xv=-cosf(owner->rot)*BULLET_SPEED;
	b1->yv=-sinf(owner->rot)*BULLET_SPEED;
	b1->owner=owner;
	b1->next=state->bulletlist;
	state->bulletlist=b1;

	b2->base.w=BULLET_WIDTH;
	b2->base.h=BULLET_HEIGHT;
	b2->base.x=(owner->x+(owner->w/2.0f)-(BULLET_WIDTH/2.0f))-(cosf(owner->rot-ANGLE_OFFSET)*DISPLACE);
	b2->base.y=(owner->y+(owner->h/2.0f)-(BULLET_HEIGHT/2.0f))-(sinf(owner->rot-ANGLE_OFFSET)*DISPLACE);
	b2->base.rot=owner->rot;
	b2->xv=-cosf(owner->rot)*BULLET_SPEED;
	b2->yv=-sinf(owner->rot)*BULLET_SPEED;
	b2->owner=owner;
	b2->next=state->bulletlist;
	state->bulletlist=b2;
}
struct bullet *deletebullet(struct state *state,struct bullet *bullet,struct bullet *prev){
	if(prev!=NULL)prev->next=bullet->next;
	else state->bulletlist=bullet->next;
	void *temp=bullet->next;
	free(bullet);
	return temp;
}

void newcloud(struct state *state){
	int count=0;
	for(struct cloud *cloud=state->cloudlist;cloud!=NULL;cloud=cloud->next)
		++count;
	if(count>12)
		return;

	while(count<5){
		struct cloud *cloud=malloc(sizeof(struct cloud));
		cloud->base.w=CLOUD_SIZE;
		cloud->base.h=CLOUD_SIZE;
		cloud->base.rot=0.0f;
		float xoffset=randomint(1.0f,11.0f)*(onein(2)?1.0f:-1.0f);
		float yoffset=randomint(1.0f,11.0f)*(onein(2)?1.0f:-1.0f);
		cloud->base.x=(state->player.base.x+(PLAYER_WIDTH/2.0f))+xoffset;
		cloud->base.y=(state->player.base.y+(PLAYER_HEIGHT/2.0f))+yoffset;
		cloud->next=state->cloudlist;
		state->cloudlist=cloud;
		++count;
	}
}
struct cloud *deletecloud(struct state *state,struct cloud *cloud,struct cloud *prev){
	if(prev!=NULL)prev->next=cloud->next;
	else state->cloudlist=cloud->next;
	void *temp=cloud->next;
	free(cloud);
	return temp;
}

