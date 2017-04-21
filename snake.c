#include <stdio.h>
#include <stdlib.h>
#include <curses.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <sys/time.h>
#include <signal.h>
#include <termios.h>
#include <fcntl.h>

#define LEFT 0
#define RIGHT COLS
#define TOP 5
#define BOTTOM (LINES - 5)
#define TITLE "HELLO SNAKE"
#define SNAKE_INIT_SIZE 5
#define POINT "*"
#define MESSAGE "********************* Game Over! *********************** \n"
#define USAGE " Usage: w/s/a/d -> up/down/left/right | space -> stop or run \n "
#define RESTARTMSG "********************* press r to restart | q to quit ********************* \n"

typedef struct _coordinate
{
	int x;
	int y;
} *pcoord;

typedef struct _node
{
	pcoord coord;
	struct _node *next;
} snake_node, *snake_head;

void setCrmode();
void enableKbdSignals();
void drawFrame();
void init();
snake_node* generateNode(int x, int y);
void drawSnake();
void initSnakeHead();
void onInput();
void moveSnake();
void setTimer();
void eraseNode();
int checkCollision(snake_node* pnode);
void hookAfterMove();
snake_node* randOneNode();

snake_head head; //蛇头
int delay = 300; //时钟延时 
char direction = 'd'; //方向 wsad 上下左右
int done = 0; //控制生命周期
int suspend = 0;
snake_node* randNode; //临时产生的坐标
int restart = 0; //是否重新开始

int main(int argc, char* argv[])
{
restartp: initscr();
	printf("start hello \n");
	clear();
	setCrmode(); // char by char
	drawFrame();
	initSnakeHead();
	srand(time(0));
	drawSnake();
	signal(SIGIO, onInput);
	signal(SIGALRM, moveSnake);
	enableKbdSignals();
	setTimer();
	while(!done)
	{
		if(restart)
		{
			restart = 0;
			goto restartp;
		}
		pause();
	}
	//return endwin();
	clear();
	refresh();
	return 0;
}

void setCrmode()
{
	struct termios ttystate;

	tcgetattr(0, &ttystate);
	ttystate.c_lflag &= ~ICANON;
	ttystate.c_lflag &= ~ECHO;
	ttystate.c_cc[VMIN] = 1;
	tcsetattr(0, TCSANOW, &ttystate);
}

void enableKbdSignals()
{
	int fd_flags;
	fcntl(0, F_SETOWN, getpid());
	fd_flags = fcntl(0, F_GETFL);
	fcntl(0, F_SETFL, (fd_flags | O_ASYNC));
}

void drawFrame()
{
	// title
	mvaddstr(TOP / 2, (RIGHT - LEFT - strlen(TITLE)) / 2, TITLE);

	int i;
	// draw topline
	for(i = LEFT; i <= RIGHT; i++)
	{
		mvaddstr(TOP, i, "-");
	}
	// draw left line
	for(i = TOP + 1; i < BOTTOM; i++)
	{
		mvaddstr(i, LEFT, "|");
	}
	// draw right line
	for(i = TOP + 1; i < BOTTOM; i++)
	{
		mvaddstr(i, RIGHT - 1, "|");
	}
	// draw bottom line
	for(i = LEFT; i <= RIGHT; i++)
	{
		mvaddstr(BOTTOM, i, "-");
	}

	// usage
	mvaddstr(BOTTOM + 2, (RIGHT - LEFT - strlen(USAGE)) / 2, USAGE);
	mvaddstr(BOTTOM + 3, (RIGHT - LEFT - strlen(RESTARTMSG)) / 2, RESTARTMSG);
	refresh();
}

/**
* 初始化蛇头
*/
void initSnakeHead()
{
	snake_head p = NULL;
	int x = TOP + (BOTTOM - TOP) / 2;
	int y = (RIGHT - LEFT) / 2;
	if(!head)
	{
		head = (snake_node *) malloc(sizeof(snake_node));
		head->next = NULL;
	}
	
	int i;
	for(i = 0; i < SNAKE_INIT_SIZE; i++)
	{
		p = (snake_node *) malloc(sizeof(snake_node));
		p->coord = (struct _coordinate *)malloc(sizeof(struct _coordinate));
		p->coord->x = x;
		p->coord->y = y;
		p->next = head->next;
		head->next = p;
		y += 2;
	}
}

/**
* 初始化 一粒蛇的食物
*/
snake_node* generateNode(int x, int y)
{
	snake_node *tmp_node = (snake_node *) malloc(sizeof(snake_node));
	tmp_node->coord = (struct _coordinate *)malloc(sizeof(struct _coordinate));
	tmp_node->coord->x = x;
	tmp_node->coord->y = y;
	tmp_node->next = NULL;

	return tmp_node;
}

/**
* 画蛇添足
*/
void drawSnake()
{
	if(!head || !head->next)
	{
		return;
	}
	if(suspend == 1)
	{
		return;
	}
	snake_node * pnode = head->next;
	while(pnode)
	{
		mvaddstr(pnode->coord->x, pnode->coord->y, POINT);
		pnode = pnode->next;
	}

	move(BOTTOM + 4, LEFT);
	refresh();
}

/**
* 处理输入
*/
void onInput()
{
	int c = getchar();
	if(c == 'q' || c == EOF){
		done = 1;
	}
	if((c == 'w' && direction != 's') || (c == 's' && direction != 'w') 
		|| (c == 'a' && direction != 'd') || (c == 'd' && direction != 'a'))
	{
		direction = c;
	}
	if(c == ' ')
	{
		suspend = suspend == 1 ? 0 : 1;
	}
	if(c == 'r')
	{
		// 恢复信号缺省
		//signal(SIGALRM, SIG_IGN);
		//signal(SIGIO, SIG_IGN);

		restart = 1;
		suspend = 0;
		// 防止内存泄露，要销毁链表
		snake_node *p = head->next, *q = head->next->next;
		while(q && q->next)
		{
			free(p->coord);
			free(p);
			p = q;
			q = q->next;
		}
		if(q)
		{
			free(q->coord);
			free(q);
		}

		// free randNode
		free(randNode->coord);
		free(randNode);
		randNode = NULL;
		head = NULL;
		randNode = NULL;
		direction = 'd';
	}
}

/**
* 移动
*/
void moveSnake()
{
	if(!head || !head->next)
	{
		return;
	}

	if(suspend == 1)
	{
		return;
	}

	//1、删除最后一个节点
	snake_node * pnode = head->next;
	while(pnode)
	{
		if(pnode->next && pnode->next->next)
		{
			pnode = pnode->next;	
		}
		else
		{
			eraseNode(pnode->next->coord->x, pnode->next->coord->y);
			free(pnode->next);
			pnode->next = NULL;
			break;
		}
	}

	//2、头部加入新的节点
	int x, y;
	switch(direction)
	{
		case 'w':
			x = head->next->coord->x - 1;
			y = head->next->coord->y;;
			break;
		case 's':
			x = head->next->coord->x + 1;
			y = head->next->coord->y;
			break;
		case 'a':
			x = head->next->coord->x;
			y = head->next->coord->y - 2;
			break;
		case 'd':
			x = head->next->coord->x;
			y = head->next->coord->y + 2;
			break;
		default:
			break;
	}

	snake_node *tmp_node = generateNode(x, y);
	tmp_node->next = head->next;
	head->next = tmp_node;

	signal(SIGALRM, moveSnake);
	hookAfterMove();
	drawSnake();
}

/**
* 定时器
*/
void setTimer()
{
	struct itimerval new_timeset;
	long n_sec, n_usecs;

	n_sec = delay / 1000; //秒
	n_usecs = (delay % 1000) * 1000L;

	new_timeset.it_value.tv_sec = n_sec;
	new_timeset.it_value.tv_usec = n_usecs;

	new_timeset.it_interval.tv_sec = n_sec;
	new_timeset.it_interval.tv_usec = n_usecs;

	setitimer(ITIMER_REAL, &new_timeset, NULL);
}

/**
* 擦除
*/
void eraseNode(int x, int y)
{
	mvaddstr(x, y, " ");
	refresh();
}

/**
* 检查碰撞
*/
int checkCollision(snake_node* pnode)
{
	//printf("pnode x:%d y:%d \n", pnode->coord->x, pnode->coord->y);
	if(!pnode || !pnode->coord || !pnode->coord->x)
	{
		return 1;
	}
	int x = pnode->coord->x;
	int y = pnode->coord->y;
	//1、node坐标在边框或者边框外 碰撞
	if(x <= TOP || x >= BOTTOM || y <= LEFT || y >= RIGHT)
	{
		return 1;
	}
	//2、node坐标在蛇身体内 碰撞 从蛇的第二个节点开始计算
	if(head->next && head->next->next)
	{
		snake_node *pTmpNode = head->next->next;
		while(pTmpNode)
		{
			if(pTmpNode->coord->x == x && pTmpNode->coord->y == y)
			{
				return 1;
			}
			pTmpNode = pTmpNode->next;
		}
	}

	return 0;
}

/**
* 随机在框框内生成一个食物
*/
snake_node* randOneNode()
{
	int x = rand() % ( BOTTOM - TOP + 1 ) + TOP;
	int y = 0;
	while(y % 2 == 0)
	{
		y = rand() % ( RIGHT - LEFT + 1) + LEFT;
	}

	return generateNode(x, y);
}

/**
* 移动后执行的钩子
*/
void hookAfterMove()
{
	if(!head || !head->next)
	{
		return;
	}

	//1、如果食物是空，就初始化一颗食物
	if(!randNode)
	{
		randNode = randOneNode();
		while(checkCollision(randNode))
		{
			randNode = randOneNode();
		}

		mvaddstr(randNode->coord->x, randNode->coord->y, "*");
		refresh();
		move(BOTTOM + 4, LEFT);

		refresh();
		//printf("x:%d y:%d \n", randNode->coord->x, randNode->coord->y);
	}

	//2、检查有没有死亡
	//printf("if collision %d\n", checkCollision(head->next));
	if(checkCollision(head->next))
	{
		//暂停游戏
		suspend = suspend == 1 ? 0 : 1;
		clear();
		mvaddstr(TOP / 2, (RIGHT - LEFT - strlen(MESSAGE)) / 2, MESSAGE);
		mvaddstr(TOP / 2 + 2, (RIGHT - LEFT - strlen(RESTARTMSG)) / 2, RESTARTMSG);
		refresh();
	}
	
	//3、检查是否吃到食物 -- 吃到食物以后，加到尾巴上
	if(head->next->coord->x == randNode->coord->x && head->next->coord->y == randNode->coord->y)
	{
		snake_node *pTmpNode = head;
		while(pTmpNode->next)
		{
			pTmpNode = pTmpNode->next;
		}
		pTmpNode->next = randNode;
		randNode = NULL;
	}

}
