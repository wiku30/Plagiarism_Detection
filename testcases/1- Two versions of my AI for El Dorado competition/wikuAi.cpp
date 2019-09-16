/****************************************
   AI Program for El Dorado
   Designed by wiku30,
   2016.03.27 ~ 2016.05.02
****************************************/

#include "console.h"
#include <vector>
#include <algorithm>
#include <math.h>
#include <time.h>
#include <stdlib.h>

using namespace std;
static const char* HERO_NAME[] = {"Hammerguard", "Berserker", "Berserker", "Scouter", "Master","Hammerguard", "Scouter", "Berserker" };
static Pos asm_pos;
static int asm_duration=0;
static int base_danger=0;
static int push_count;
static const PUnit *base;
static int camp;
static int group;
static PUnit* cursor;
static int roshandead=0;
static int dragondead=0;

static int push_sign=0;
static int push_wait=0;
static int ebasehp=3000;

static Pos wait[]={Pos(45,120),Pos(105,30)};
const Pos eye[]={Pos(25,15),Pos(125,135)};
const Pos mine[] = {Pos(74, 75), Pos(43, 61),Pos(43, 89), Pos(20, 140) , Pos(107, 89), Pos(107, 61), Pos(130, 10) };
const Pos ready[]={Pos(118,136),Pos(32,14)};
const int VIOLENT_PUSH_BOUND=1000;
const int FARM_BOUND=2200;
const int FARM_TURNS=600;
const int CALLBACK_BOUND=5;
const int UPGRADE_RESERVE=400;

inline bool inrange(const PUnit *attacker,const PUnit *defender)
{
    return dis2(attacker->pos,defender->pos)<=attacker->range;
}

int priority(PUnit *u)
{
    if(u->isBase()) return -19971019;
    int x=u->hp>0 ? u->hp : 19971019;
    if(!u->isHero()) x+=524288;
    if(inrange(u,base))
        x-=524288;
    if(!inrange(cursor,u))
    {
        if(u->findBuff("WinOrDie")!=NULL)
            x+=262144;
        if(cursor->findBuff("WinOrDie")!=NULL)
            x+=131072;
        if(u->speed >= cursor->speed)
            x+=65536;
        x+=(u->speed-9)*10;
    }
    return x;
}

inline int cmp(PUnit* a,PUnit* b)
{
    return priority(a)<priority(b);
}

inline int bpri(PUnit *u)
{
    int res=priority(u);
    if(u->name[0]=='B')
        res-=1024;
    return res;
}

inline int basecmp(PUnit* a,PUnit* b)
{
    return bpri(a)<bpri(b);
}

class wiku_ai
{
private:
    Console *console;
    const PMap &map;
    const PPlayerInfo &info;
    PCommand &cmd;
    int HeroNum;
    vector<PUnit*> heroes;
    vector<PUnit*> myUnits;
    vector<PUnit*> targets;
    vector<int> mindist2;
    vector<int> aflag;  //can attack
    vector<bool> overwhelmed;
    vector<PUnit*> baseenemy;
    vector<int>ally_num;
    vector<int>enemy_num;
    double total_level;
    bool asm_flag;
    enum{no=0,major=1,all=2} asm_stat;
    bool pushing_flag;
    bool ready_flag;
public:
    wiku_ai(const PMap &map, const PPlayerInfo &info, PCommand &cmd);
    ~wiku_ai(){delete console;}
    inline void attack(int who);
    inline void dodge(int who);
    inline void think(int who);
    inline bool dodgecondition(int who);
    void get_heroes();
    void get_targets();
    void assembly(vector<PUnit*> team,int n=1000);
    void cast(int who,const char* skill, int range);
    void blink(int who, Pos where);
    void blinkdir(int who);
    void blinkdir(int who, Pos where);
    void sacrifice_check(int who);
    void occupy(int who);
    void assembly(int who);
    void initialize();
    void base_check();
    void push_check(int i);
    void push(int who);
    void upgrade(vector<PUnit*> units);
    void work();
    Pos wildmine(int who);
};



wiku_ai::wiku_ai(const PMap &_map, const PPlayerInfo &_info, PCommand &_cmd):map(_map),info(_info),cmd(_cmd)
{
    console=new Console(map,info,cmd);
    asm_stat=no;
    asm_flag=0;
    pushing_flag=0;
    ready_flag=0;
    roshandead=0;
    total_level=0;
    camp=console->camp();
    base=console->getMilitaryBase();
    vector<PUnit*> myUnits = console->friendlyUnits();
}

void wiku_ai::initialize()
{
    myUnits = console->friendlyUnits();
	HeroNum=0;
	vector<PUnit*> allenemy= console->enemyUnits();
    for(int i = 0; i < myUnits.size(); ++i)
	    if(myUnits[i]->isHero())
        {
            HeroNum++;
            total_level+=myUnits[i]->level;
        }
    if(!HeroNum)
        for(int w=0;w<4;w++)
            console->chooseHero(HERO_NAME[w]);
    else
        console->chooseHero(HERO_NAME[(HeroNum)]);
    myUnits = console->friendlyUnits();

	if(HeroNum>=8)
    {
        if(push_sign==0)
            push_sign=1;
    }
    int tmp=-1;
    upgrade(myUnits);
    int r_flag=0,d_flag=0;
	for(int i=0;i<allenemy.size();i++)
    {
        if(allenemy[i]->name[0]=='R' && allenemy[i]->findBuff("Reviving") && dis2(allenemy[i]->pos,base->pos)<10000)
        {
            roshandead=allenemy[i]->findBuff("Reviving")->timeLeft;
            r_flag=1;
        }
        if(allenemy[i]->name[0]=='D' && allenemy[i]->findBuff("Reviving") && dis2(allenemy[i]->pos,base->pos)<10000)
        {
            dragondead=allenemy[i]->findBuff("Reviving")->timeLeft;
            d_flag=1;
        }
    }
    if(!r_flag && roshandead>0)
        roshandead--;
    if(!d_flag && dragondead>0)
        dragondead--;
}

void wiku_ai::get_heroes()
{
    for(int who = 0; who < myUnits.size(); ++who)
    {
        if(myUnits[who]->isHero() && !myUnits[who]->findBuff("Reviving"))
        {
            int align=dis2(myUnits[who]->pos,base->pos)<400?3:1;
            heroes.push_back(myUnits[who]);
            console->selectUnit(myUnits[who]);
            UnitFilter filter;
            filter.setAreaFilter (new Circle (myUnits[who]->pos, myUnits[who]->view * align), "a");
            filter.setAvoidFilter ("Mine");
            vector<PUnit*> ally = console->friendlyUnits(filter);
            ally_num.push_back(ally.size());
            aflag.push_back(1);
        }
    }
}

void wiku_ai::get_targets()
{
    UnitFilter filter;
    for(int i=0;i<heroes.size();i++)
    {
        int align=dis2(heroes[i]->pos,base->pos)<400?3:1;
        filter.setAreaFilter (new Circle (heroes[i]->pos, heroes[i]->view * align), "a");
        filter.setAvoidFilter ("Mine");
        vector<PUnit*> enemy = console->enemyUnits(filter);
        enemy_num.push_back(enemy.size());
        sort(enemy.begin(),enemy.end(),cmp);
        while((enemy.size() && enemy[0]->findBuff("WinOrDie")!=NULL) &&
              (dis2(enemy[0]->pos,heroes[i]->pos)>heroes[i]->range)
              && (!heroes[i]->canUseSkill("HammerAttack")))
            enemy.erase(enemy.begin());
        while(enemy.size() && (enemy[0]->hp<=0))
            enemy.erase(enemy.begin());
        while(enemy.size() && (group || push_sign==1 || info.round>FARM_TURNS) && !push_wait && (enemy[0]->isWild()))
            enemy.erase(enemy.begin());
        while(enemy.size() && !enemy[0]->name[0]=='O')
            enemy.erase(enemy.begin());
        if(enemy.size()>0)
        {
            targets.push_back(enemy[0]);
            if(enemy[0]->isBase())
                pushing_flag=1;
        }
        else
            targets.push_back(NULL);
        int d_min=1600;
        for(int k=0;k<enemy.size();k++)
        {
            int d2=dis2(enemy[k]->pos,heroes[i]->pos);
            if(d2<d_min)
                d_min=d2;
        }
        mindist2.push_back(d_min);
        overwhelmed.push_back(enemy.size()>(ally_num[i]+1) || enemy.size()>int(1.4*ally_num[i]));
    }
}

void wiku_ai::upgrade(vector<PUnit*> units)
{
    for(int i=0;i<units.size();i++)
    {
        if(units[i]->isHero())
        {
            if(HeroNum>=8 || units[i]->name[0]=='B' && (units[i]->level<1))
            {
                if(units[i]->level<=total_level/8+1
                   && console->gold()>=console->levelUpCost(units[i]->level)+UPGRADE_RESERVE)
                    console->buyHeroLevel(units[i]);
            }
        }
    }
}

inline void wiku_ai::attack(int who)
{
    if(heroes.size()<=6 && ally_num[who]<enemy_num[who] && ally_num[who]>2 && group==1)
        group=0;
    if(targets[who]->isBase())
    {
        ebasehp=targets[who]->hp;
        if(targets[who]->hp>FARM_BOUND)
            push_sign=2;
        else
            push_sign=1;
        if(dis2(heroes[who]->pos,targets[who]->pos)<=36 && targets[who]->isBase())
        {
            group=0;
            push_count=0;
            if(info.round<750)
                push_wait=(800-info.round)*0.2;
        }
    }
    if(targets[who]->isWild())
    {
        group=0;
        push_count=0;
    }
    if(aflag[who] && targets[who] && (!group||base_danger||targets[who]->isBase()||inrange(heroes[who],targets[who])))
    {
        console->attack(targets[who],heroes[who]);
    }
    else
    {
        occupy(who);
    }

}

void wiku_ai::cast(int who, const char* skill, int range)
{
    if(skill[0]=='S' && heroes[who]->canUseSkill(skill))
    {
        int i=-6*console->camp()+3;
        console->useSkill(skill,heroes[who]->pos+Pos(i,i),heroes[who]);
        aflag[who]=0;
    }
    if(overwhelmed[who] && skill[0]!='B' && !targets[who]->isBase())
        return;
    if(heroes[who]->canUseSkill(skill) && (dis2(heroes[who]->pos,targets[who]->pos)<=range))
    {
        if(skill[0]=='H' && info.round>200)
        {
            if(targets[who]->isWild() && heroes[who]->mp<80 )
                return;
            if(targets[who]->isBase())
                return;
        }
        console->useSkill(skill,targets[who],heroes[who]);
        aflag[who]=0;
    }
}

void wiku_ai::blink(int who, Pos where)
{
    if(map.getHeight(where.x,where.y)>40)
        return;
    if(heroes[who]->canUseSkill("Blink") && (dis2(heroes[who]->pos,where)<=81))
    {
        console->useSkill("Blink",where,heroes[who]);
        aflag[who]=0;
    }
}

void wiku_ai::blinkdir(int who,Pos where)
{
    if(!aflag[who] || !heroes[who]->canUseSkill("Blink"))
        return;
    double x=0,y=0,xct,yct;
    x=where.x-heroes[who]->pos.x;
    y=where.y-heroes[who]->pos.y;
    srand(time(0));
    double lambda=9/sqrt(x*x+y*y);
    if(lambda>7.5) return;
    if(lambda>2.7)lambda=2.7;
    xct=int(lambda*x-0.5);
    yct=int(lambda*y-0.5);
    for(int trial=0;trial<5 && aflag[who];trial++)
    {
        x=xct+rand()%3;
        y=yct+rand()%3;
        Pos res=heroes[who]->pos+Pos(x,y);
        blink(who,res);
    }
}


void wiku_ai::blinkdir(int who)
{
    if(!aflag[who]|| !heroes[who]->canUseSkill("Blink"))
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
            for(int trial=0;trial<5 && aflag[who];trial++)
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
    int disb=dis2(heroes[who]->pos,base->pos);
    if(targets[who]->isWild() && ally_num[who]<=1 && heroes[who]->hp < targets[who]->hp*2 && dis2(heroes[who]->pos,targets[who]->pos)<=121)
        return 1;
    if(dist <= 36 || (disb <= 144 && targets[who] && dis2(targets[who]->pos,base->pos)<=49))
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
        double k=heroes[who]->hp<150 ? 13-heroes[who]->hp/15 : 3;
        if(ready_flag)
        {
            console->move(wait[console->camp()],heroes[who]);
            aflag[who]=0;
        }
        if(pushing_flag || group || dis2(heroes[who]->pos,MILITARY_BASE_POS[1-console->camp()])<1225)
        {
            console->move(MILITARY_BASE_POS[1-console->camp()],heroes[who]);
            aflag[who]=0;
        }
        if(heroes[who]->findBuff("BeAttacked")!=NULL && heroes[who]->hp<180 && heroes[who]->canUseSkill("Blink"))
            blinkdir(who);
        if(aflag[who])
        {
            Pos delta(MILITARY_BASE_POS[console->camp()].x-heroes[who]->pos.x,MILITARY_BASE_POS[console->camp()].y-heroes[who]->pos.y);
            double dist2=sqrt(delta.x*delta.x+delta.y*delta.y);
            aflag[who]=0;
            console->move(heroes[who]->pos+Pos(k*delta.x/dist2+0.5,k*delta.y/dist2+0.5),heroes[who]);
        }
    }
}

void wiku_ai::sacrifice_check(int who)
{
    if(heroes[who]->findBuff("BeAttacked")==NULL && targets[who]->isHero() && console->getSkill("Attack",heroes[who])->cd<=1)
        if(ally_num[who]>enemy_num[who] && enemy_num[who]>1)
            cast(who,"Sacrifice",heroes[who]->range);
    if(targets[who]->isWild() && targets[who]->findBuff("Dizzy")!=NULL)
        cast(who,"Sacrifice",heroes[who]->range);
    if(targets[who]->isBase() && heroes[who]->findBuff("BeAttacked")==NULL && console->getSkill("Attack",heroes[who])->cd<=1)
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
        cast(who,"HammerAttack",36);
        sacrifice_check(who);
        if(dodgecondition(who))
            dodge(who);
        else
        {
            attack(who);
        }
    }
}

void wiku_ai::base_check()
{
    UnitFilter filter;
    filter.setAreaFilter (new Circle (base->pos, base->view*3), "a");
    filter.setAvoidFilter ("Mine");
    UnitFilter filter_atk;
    filter_atk.setAreaFilter (new Circle (base->pos, base->view), "a");
    filter_atk.setAvoidFilter ("Mine");
    vector<PUnit*> enemy = console->enemyUnits(filter);
    vector<PUnit*> enemy_in = console->enemyUnits(filter_atk);
    while(enemy.size() && enemy[0]->hp<=0)
        enemy.erase(enemy.begin());
    while(enemy_in.size() && enemy[0]->hp<=0)
        enemy_in.erase(enemy.begin());
    if(enemy_in.size())
    {
        sort(enemy_in.begin(),enemy_in.end(),basecmp);
        console->baseAttack(enemy_in[0]);
    }
    if(enemy_in.size()>=CALLBACK_BOUND)
    {
        for(int i=0;i<myUnits.size();i++)
        {
            if(dis2(myUnits[i]->pos,base->pos)>1000 && dis2(myUnits[i]->pos,base->pos)>1000
                && dis2(myUnits[i]->pos,MILITARY_BASE_POS[1-console->camp()])>1000)
            {
                console->callBackHero(myUnits[i]);
            }
        }
    }
    int sz=enemy.size();
    if(sz)
    {
        base_danger+=(sz-1)*(sz-1);
    }
    if(base_danger>0 && !base->findBuff("BeAttacked"))
    {
        base_danger*=9;
        base_danger/=10;
    }
    if(base_danger>=40 || sz>=4)
    {
        asm_pos=base->pos;
        asm_flag=1;
        asm_duration=15;
    }
    if(base_danger>150)
    {
        base_danger=150;
    }
}

void wiku_ai::occupy(int who)
{
    if(!aflag[who])
        return;
    UnitFilter filter;
    filter.setAreaFilter (new Circle (heroes[who]->pos, heroes[who]->view ), "a");
    filter.setAvoidFilter ("Mine");
    vector<PUnit*> ally = console->friendlyUnits(filter);
    if(ally.size()<=1 && info.round>100 && 0)
    {
        console->move(wait[console->camp()],heroes[who]);
    }
    else if((info.round<=FARM_TURNS && push_sign!=1 || push_wait>0) && dis2(heroes[who]->pos,MILITARY_BASE_POS[1-console->camp()])>1500)
    {
        console->move(wildmine(who),heroes[who]);
    }
    else
    {
        push(who);
    }
}

void wiku_ai::assembly(vector<PUnit*> team,int n)
{
    if(group)
        return;
    push_count=0;
    for(int i=0;(i<team.size() && i<n);++i)
    {
        int disb=dis2(asm_pos,team[i]->pos);
        int dise=dis2(MILITARY_BASE_POS[1-console->camp()],team[i]->pos);
        if(disb>81 && dise>64 && (team[i]->findBuff("BeAttacked")==NULL || (targets[i] && !targets[i]->isHero())))
        {
            if(disb<15000)
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
    get_heroes();
    base_check();
    get_targets();
    if(heroes.size()<=4)
    {
        group=0;
        push_count=0;
        if(info.round<750 && (!heroes.size() || dis2(heroes[0]->pos,MILITARY_BASE_POS[1-console->camp()])<1600))
            push_wait=(800-info.round)*0.25;
    }
    if(asm_duration>0)
    {
        assembly(heroes,8);
        if(!asm_flag)
            asm_duration--;
    }
    if(push_wait>0)
        push_wait--;
    for(int i=0;i<heroes.size();i++)
    {
        think(i);
    }
}

void wiku_ai::push_check(int i)
{
    int dist=dis2(heroes[i]->pos,MILITARY_BASE_POS[1-console->camp()]);
    if(((dist<400 && ally_num[i]>=6 && ally_num[i]==heroes.size() || dist<=144 ||(pushing_flag && dist<1600) ||ebasehp<VIOLENT_PUSH_BOUND && group)&& dist> heroes[i]->range))
    {
        console->move(MILITARY_BASE_POS[1-console->camp()],heroes[i]);
        aflag[i]=0;
        if(push_count>0 && dist<=400)
        {
            push_count=0;
        }
    }
}

void wiku_ai::push(int who)
{
    UnitFilter filter;
    if(group!=2)
    {
        filter.setAreaFilter (new Circle (ready[console->camp()], 36), "a");
        filter.setAvoidFilter ("Mine");
    }
    vector<PUnit*> ally = console->friendlyUnits(filter);
    int dist=dis2(heroes[who]->pos,MILITARY_BASE_POS[1-console->camp()]);
    if(ally_num[who]>=HeroNum && group==0 && dis2(heroes[who]->pos,wait[console->camp()])<25 || (ally_num[who]>=4 && dist<2500))
        group=1;
    if(ally_num[who]>=7 && dist<1225)
        group=2;
    if((group==2 || (group==0  && dist<1225)) || push_count>5000)
    {
        console->move(MILITARY_BASE_POS[1-console->camp()],heroes[who]);
        blinkdir(who,MILITARY_BASE_POS[1-console->camp()]);
    }
    else if(group==1 || push_count>400)
    {
        console->move(ready[console->camp()],heroes[who]);
        blinkdir(who,ready[console->camp()]);
        if(ally_num[who]>=5)
            push_count+=10*(ally_num[who]-4);
    }
    else
    {
        console->move(wait[console->camp()],heroes[who]);
        blinkdir(who,wait[console->camp()]);
        if(ally_num[who]>=5 && push_count<400)
            push_count+=(ally_num[who]-4); //5人+5,6人+12，7人+21
    }
}

Pos wiku_ai::wildmine(int who)
{
    int sz=heroes.size();
    int k;
    if(sz>=7 && info.round>450 && base->hp>=2500-info.round && 0)
        k=3;
    else if(sz>=7 && info.round>400 && base->hp>=1500-info.round/2 || roshandead>5 || dragondead>5 && sz>=5 && info.round>200)
        k=2;
    else
        k=1;
    int n;
    if(k==1)
        n=console->camp()*3+1;
    else if(who>=sz*0.35 || who>=sz*0.65 && roshandead>5 || dragondead>5 && who>0)
        n=console->camp()*3+2;
    else
        n=console->camp()*3+1;
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
