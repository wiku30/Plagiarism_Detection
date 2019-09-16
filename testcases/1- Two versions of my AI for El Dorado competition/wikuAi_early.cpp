#include "console.h"
#include <vector>
#include <algorithm>
#include <math.h>
#include <time.h>
#include <stdlib.h>

using namespace std;
static const char* HERO_NAME[] = {"Hammerguard", "Scouter", "Master", "Berserker"};
static Pos asm_pos;
static int asm_duration=0;
static int base_danger=0;
static int push_count=0;
static PUnit* cursor;

static const Pos wait[]={Pos(45,45),Pos(105,105)};
static const Pos eye[]={Pos(25,15),Pos(125,135)};
static const Pos mine[] = {Pos(74, 75), Pos(35, 60), Pos(35, 90), Pos(130, 10), Pos(115, 90), Pos(115, 60), Pos(20, 140)};

inline bool inrange(PUnit *attacker,PUnit *defender)
{
    return dis2(attacker->pos,defender->pos)<=attacker->range;
}

int priority(PUnit *u)
{
    if(u->isBase()) return -19971019;
    int x=u->hp>0 ? u->hp : 19971019;
    if(!u->isHero()) x+=524288;
    if(!inrange(cursor,u))
    {
        if(u->findBuff("WinOrDie")!=NULL)
            x+=262144;
        if(cursor->findBuff("WinOrDie")!=NULL)
            x+=131072;
        if(u->speed >= cursor->speed)
            x+=65536;
        x+=(u->speed-5)*20;
    }
    return x;
}

inline int cmp(PUnit* a,PUnit* b)
{
    return priority(a)<priority(b);
}

class wiku_ai
{
private:

    Console *console;
    const PMap &map;
    const PPlayerInfo &info;
    PCommand &cmd;

    vector<PUnit*> heroes;
    vector<PUnit*> targets;
    vector<int> mindist2;
    vector<int> aflag;  //can attack
    vector<bool> overwhelmed;
    vector<PUnit*> baseenemy;
    vector<int>ally_num;
    vector<int>enemy_num;
    PUnit *base;
    bool asm_flag;
    enum{no=0,major=1,all=2} asm_stat;
public:
    wiku_ai(const PMap &map, const PPlayerInfo &info, PCommand &cmd);
    ~wiku_ai(){delete console;}
    inline void attack(int who);
    inline void dodge(int who);
    inline void think(int who);
    inline bool dodgecondition(int who);
    void assembly(vector<PUnit*> team,int n=1000);
    void cast(int who,const char* skill, int range);
    void blink(int who, Pos where);
    void blinkdir(int who);
    void sacrifice_check(int who);
    void occupy(int who);
    void assembly(int who);
    void initialize();
    void base_check();
    void push_check(int i);
    void work();
    Pos wildmine(int who);
};

wiku_ai::wiku_ai(const PMap &_map, const PPlayerInfo &_info, PCommand &_cmd):map(_map),info(_info),cmd(_cmd)
{
    console=new Console(map,info,cmd);
    asm_stat=no;
    asm_flag=0;
}

void wiku_ai::initialize()
{
    vector<PUnit*> myUnits = console->friendlyUnits();
	int HeroNum=0;
    for(int i = 0; i < myUnits.size(); ++i)
	    if(myUnits[i]->isHero())
            HeroNum++;
    console->chooseHero(HERO_NAME[(HeroNum) % 4]);
    myUnits = console->friendlyUnits();
	if(HeroNum>=8)
		for(int i = 0; i < myUnits.size(); ++i)
			console->buyHeroLevel(myUnits[i]);
    int tmp=-1;
    for(int who = 0; who < myUnits.size(); ++who)
    {
        if(myUnits[who]->isHero() && myUnits[who]->hp>0)
        {
            tmp++;
            int align=dis2(myUnits[who]->pos,base->pos)<400?3:1;
            heroes.push_back(myUnits[who]);
            console->selectUnit(myUnits[who]);
            UnitFilter filter;
            filter.setAreaFilter (new Circle (myUnits[who]->pos, myUnits[who]->view * align), "a");
            filter.setAvoidFilter ("Mine");
            vector<PUnit*> enemy = console->enemyUnits(filter);
            vector<PUnit*> ally = console->friendlyUnits(filter);
            ally_num.push_back(ally.size());
            enemy_num.push_back(enemy.size());
            sort(enemy.begin(),enemy.end(),cmp);
            while((enemy.size() && enemy[0]->findBuff("WinOrDie")!=NULL) &&
                  (dis2(enemy[0]->pos,heroes[tmp]->pos)>heroes[tmp]->range)
                  && (heroes[tmp]->canUseSkill("HammerAttack")==NULL))
                enemy.erase(enemy.begin());
            while(enemy.size() && (enemy[0]->hp<=0))
                enemy.erase(enemy.begin());
            if(enemy.size()>0)
                targets.push_back(enemy[0]);
            else
                targets.push_back(NULL);
            int d_min=1600;
            for(int k=0;k<enemy.size();k++)
            {
                int d2=dis2(enemy[k]->pos,heroes[tmp]->pos);
                if(d2<d_min)
                    d_min=d2;
            }
            mindist2.push_back(d_min);
            overwhelmed.push_back(enemy.size()>(ally.size()+1) || enemy.size()>int(1.4*ally.size()));
            aflag.push_back(1);
        }
        else if(myUnits[who]->isBase())
            base=myUnits[who];
    }
    if(tmp==-1)
        throw(1);
}

inline void wiku_ai::attack(int who)
{
    if(aflag[who])
        console->attack(targets[who],heroes[who]);
}

void wiku_ai::cast(int who, const char* skill, int range)
{
    if(skill[0]=='S' && heroes[who]->canUseSkill(skill))
    {
        int i=-6*console->camp()+3;
        console->useSkill(skill,heroes[who]->pos+Pos(i,i),heroes[who]);
        aflag[who]=0;
    }
    if(overwhelmed[who] && skill[0]!='B')
        return;
    if(heroes[who]->canUseSkill(skill) && (dis2(heroes[who]->pos,targets[who]->pos)<=range))
    {
        console->useSkill(skill,targets[who],heroes[who]);
        aflag[who]=0;
    }
}

void wiku_ai::blink(int who, Pos where)
{
    if(heroes[who]->canUseSkill("Blink") && (dis2(heroes[who]->pos,where)<=81))
    {
        console->useSkill("Blink",where,heroes[who]);
        aflag[who]=0;
    }
}

void wiku_ai::blinkdir(int who)
{
    if(!aflag[who])
        return;
    double x=0,y=0,xct,yct;
    UnitFilter filter; //建立筛选器
    filter.setAreaFilter (new Circle (heroes[who]->pos, heroes[who]->view ), "a");
    filter.setAvoidFilter ("Mine");
    vector<PUnit*> ally = console->friendlyUnits(filter);
    for(int i=0;i<ally.size();i++)
    {
        x+=ally[i]->pos.x;
        y+=ally[i]->pos.y;
    }
    x/=ally.size();x-=heroes[who]->pos.x;
    y/=ally.size();y-=heroes[who]->pos.y;
    if(ally.size()>1)
    {
        srand(time(0));
        double lambda=9/sqrt(x*x+y*y);
        if(lambda>7.5) return;
        if(lambda>2.7)lambda=2.7;
        xct=int(lambda*x-0.5);
        yct=int(lambda*y-0.5);
            for(int trial=0;trial<5;trial++)
            {
                x=xct+rand()%3;
                y=yct+rand()%3;
                Pos res=heroes[who]->pos+Pos(x,y);
                blink(who,res);
            }
    }
}

inline bool wiku_ai::dodgecondition(int who)
{
    int dist=dis2(heroes[who]->pos,MILITARY_BASE_POS[1-console->camp()]);
    if(dist <= 36)
        return 0;
    if(heroes[who]->findBuff("WinOrDie")!=NULL)
        return 0;
    if(heroes[who]->hp<60 || overwhelmed[who])
        return 1;
    if((heroes[who]->range>30 && heroes[who]->hp<230))
        if(mindist2[who]<13)
            return 1;
    return 0;
}

void wiku_ai::dodge(int who)
{
    if(aflag[who])
    {
        if(heroes[who]->findBuff("BeAttacked")!=NULL && heroes[who]->canUseSkill("Blink"))
            blinkdir(who);
        if(aflag[who])
        {
            Pos delta(MILITARY_BASE_POS[console->camp()].x-heroes[who]->pos.x,MILITARY_BASE_POS[console->camp()].y-heroes[who]->pos.y);
            double dist2=sqrt(delta.x*delta.x+delta.y*delta.y);
            aflag[who]=0;
            console->move(heroes[who]->pos+1.5*Pos(delta.x/sqrt(dist2),delta.y/sqrt(dist2)),heroes[who]);
        }
    }
}

void wiku_ai::sacrifice_check(int who)
{
    if(heroes[who]->findBuff("BeAttacked")==NULL && targets[who]->isHero() && heroes[who]->canUseSkill("Attack"))
        if(ally_num[who]>enemy_num[who])
            cast(who,"Sacrifice",heroes[who]->range);
}

inline void wiku_ai::think(int who)
{
    cursor=heroes[who];
    push_check(who);
    if(!aflag[who])
        return;
    if(targets[who]==NULL)
        occupy(who);
    else
    {
        push_count=0;
        cast(who,"HammerAttack",36);
        sacrifice_check(who);
        if(dodgecondition(who))
            dodge(who);
        else
        {
            push_count=0;
            attack(who);
        }
    }
}

void wiku_ai::base_check()
{
    UnitFilter filter;
    filter.setAreaFilter (new Circle (base->pos, base->view*3), "a");
    filter.setAvoidFilter ("Mine");
    vector<PUnit*> enemy = console->enemyUnits(filter);
    while(enemy.size() && enemy[0]->hp<=0)
        enemy.erase(enemy.begin());
    int sz=enemy.size();
    if(sz)
    {
        sort(enemy.begin(),enemy.end(),cmp);
        console->baseAttack(enemy[0]);
        base_danger+=sz*(sz+1)/2;
    }
    else if(base_danger>0)
    {
        base_danger*=24;
        base_danger/=25;
    }
    if(base_danger>=16)
    {
        asm_pos=base->pos;
        asm_flag=1;
        asm_duration=25;
    }
    if(base_danger>200)
    {
        base_danger=200;
    }
}

void wiku_ai::occupy(int who)
{
    UnitFilter filter;
    filter.setAreaFilter (new Circle (heroes[who]->pos, heroes[who]->view ), "a");
    filter.setAvoidFilter ("Mine");
    vector<PUnit*> ally = console->friendlyUnits(filter);
    if(ally.size()<=1 && info.round>100)
    {
        console->move(wait[console->camp()],heroes[who]);
    }
    else if((info.round>550 && ally.size()<=6 && push_count <120))
    {
        console->move(MINE_POS[0],heroes[who]);
        push_count++;
    }
    else if(info.round<=550)
    {
        console->move(wildmine(who),heroes[who]);
    }
    else
    {
        console->move(MILITARY_BASE_POS[1-console->camp()],heroes[who]);
    }
}

void wiku_ai::assembly(vector<PUnit*> team,int n)
{
    for(int i=0;(i<team.size() && i<n);++i)
    {
        int disb=dis2(asm_pos,team[i]->pos);
        int dise=dis2(MILITARY_BASE_POS[1-console->camp()],team[i]->pos);
        if(disb>100 && dise>64 && (team[i]->findBuff("BeAttacked")==NULL || (targets[i] && !targets[i]->isHero())))
        {
            if(disb<19600)
                console->move(asm_pos,team[i]);
            else
                console->move(MILITARY_BASE_POS[1-console->camp()],team[i]);
            aflag[i]=0;
        }
    }
}

void wiku_ai::work()
{
    initialize();
    int help=0;
    base_check();
    if(asm_duration>0)
    {
        assembly(heroes,8);
        if(!asm_flag)
            asm_duration--;
    }
    for(int i=0;i<heroes.size();i++)
    {
        think(i);
    }
}

void wiku_ai::push_check(int i)
{
    int dist=dis2(heroes[i]->pos,MILITARY_BASE_POS[1-console->camp()]);
    if(((dist<625 && ally_num[i]>=4 || dist<=144)&& dist> heroes[i]->range))
    {
        console->move(MILITARY_BASE_POS[1-console->camp()],heroes[i]);
        aflag[i]=0;
        if(push_count>0 && dist<=400)
            push_count=0;
    }
}

Pos wiku_ai::wildmine(int who)
{
    int sz=heroes.size();
    int k;
    if(sz>=6 && info.round>300 && 0)
        k=3;
    else if(sz>=4 && info.round>150)
        k=2;
    else
        k=1;
    int n=console->camp()*3+1+who*k/sz;
    return mine[n];
}

void player_ai(const PMap &map, const PPlayerInfo &info, PCommand &cmd)
{
    try
    {
        wiku_ai ai(map,info,cmd);
        ai.work();
    }
    catch(int)
    {
        return;
    }
}
