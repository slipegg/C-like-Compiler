#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "dialog.h"
#include "dialog2.h"
#include "dialog3.h"
#include "para.h"
#include <QPainter>
#include <QPixmap>
#include<iostream>
#include <fstream>
#include<string.h>
#include<QTextCodec>

#define _CRT_SECURE_NO_WARNINGS
#include<iostream>
#include<algorithm>
#include<stdio.h>
#include<fstream>
#include<iomanip>
#include<string.h>
#include<sstream>
#include<stack>
using namespace std;
#define MAX 500
#define LM 50
#define TXT_MAX 10000
#define VN 0
#define VT 1
#define GO 0
#define S 1
#define R 2
struct V//终结符或非终结符
{
    string name;//符号名字
    bool type;//0代表非终结符，1 代表终结符
    string realName;//记录真实的变量名和字符串名
    bool operator==(const V b) const//重构==，判断2个V是否相等
    {
        return this->name == b.name && this->type == b.type;
    }
};
struct STR//推导式
{
    V left;//左边符号
    V right[MAX];//右边符号
    int right_len = 0;//右边符号的长度
};
struct FF//first集合
{
    V name;//对应的非终结符名字
    V first_list[MAX];//first列表
    int first_list_len = 0;//first列表的长度
};
struct Item//项目
{
    STR expression;//推导式
    V forward[LM];//展望字符
    int forward_len = 0;//展望字符长度
    int index = 0;//当前右部坐标
};

struct itemChange//项目的移动信息
{
    int target;//移动目标
    V method;//借助的符号
};

struct Itemset//项目集
{
    Item item[LM];//项目列表
    int item_len = 0;//项目列表长度
    itemChange change[LM];//该项目集转换到其他项目集的方式和目标
    int change_len = 0;//change数组的有效长度
};

struct ACTION_GOTO//ACTION_GOTO表的一个单元
{
    V method;//借助的符号
    int action;//动作，go:0 s:1 r:2
    int target;//目标
};

struct ALL_ACTION_GOTO//ACTION_GOTO表的一行
{
    ACTION_GOTO* ag;//这一行中的所有有效单元
    int len = 0;//这一行中的所有有效单元的长度
};

struct FourTranslate
{
    int id;//第几个表达式
    string one;//第一个值
    string two;//第二个值
    string three;//第三个值
    string four;//第四个值
};

#define TXTLEN 100000
#define OPLEN 36
#define ONEOPLEN 28
#define TWOOPLEN 8
#define IDLEN 1000
#define WORDLEN 44



//全局变量，保留字表
static char reserveWord[WORDLEN][20] = {
    "include","using","namespace","std","define",
    "main","bool","auto", "break", "case", "char", "const", "continue",
    "cout","cin","_getch","default", "do", "double", "else", "enum", "extern", "while",
    "float", "for", "goto", "if", "int", "long","string",
    "register", "return", "short", "signed", "sizeof", "static",
    "struct", "switch", "typedef", "union", "unsigned", "void",
    "volatile","system"
};
//运算符表
static char oneOpera[ONEOPLEN] = {
'+', '-', '*', '/', '<',  '>',  '=',
';', '(', ')', '^', ',','\"', '\'', '#', '&',
'|',  '%', '~',  '[', ']', '{','\\',
'}' , '.', ':', '!','\?',
};
static char twoOpera[TWOOPLEN][3] = {
     "<=", ">=",  "==",
    "!=","&&","||","<<", ">>"
};

static  char IDentifierTbl[IDLEN][100] = { "" };//标识符表
char IDstr[IDLEN][1000] = { "" };
static int ti=0;
static bool isStr = false;
static bool isYinhao = false;
static int isChar = false;
static int isInclude = 0;
//读取文件
string intToString(int a)
{
    stringstream ss;
    string res;
    ss << a;
    ss >> res;
    return res;
}
int stringToInt(char* a)
{
    stringstream ss;
    int res;
    ss << a;
    ss >> res;
    return res;
}
char* read(char* path, int& len) {
    static char content[TXTLEN];
    FILE* fp;
    if ((fp = fopen(path, "r")) == NULL) {
        cout << "文件打开失败！" << endl;
    }

    fseek(fp, 0L, SEEK_END);
    len = ftell(fp);
    rewind(fp);
    fread(content, 1, len, fp);
    /*if (fread(content, 1, len, fp) != len) {
        cout << "读取失败" << endl;
    }*/
    fclose(fp);

    return content;
}

//剔除注释语句
void del_zs(char* txt) {
    char* new_txt = new char[TXTLEN];
    int ni = 0;
    int tlen = strlen(txt);
    for (int i = 0; i < tlen; i++) {
        if (txt[i] == '/' && txt[i + 1] == '/') {
            while (txt[i] != '\n'&&i<tlen) {
                i++;
            }
        }
        else if (txt[i] == '/' && txt[i + 1] == '*') {
                i += 2;//避免/*/的情况
                while (txt[i] != '*'&&txt[i+1]!='/' && i < tlen) {
                    i++;
                }
                i += 2;
            }
            new_txt[ni++] = txt[i];
        //}
    }
    new_txt[ni] = '\0';
    strcpy(txt, new_txt);
}

void del_other(char* txt) {
    char* new_txt = new char[TXTLEN];
    int ni = 0;
    int tlen = strlen(txt);

    for (int i = 0; i < tlen; i++) {
        if (txt[i] == ' ') {
            while (txt[i] == ' '){
                i++;
            }
            i--;
        }
        else if(txt[i] == '\n'||int(txt[i])==9){
            while (txt[i] == '\n'||txt[i]==' '||int(txt[i])==9) {
                i++;
            }
        }
        new_txt[ni++] = txt[i];
    }
    new_txt[ni] = '\0';
    strcpy(txt, new_txt);
}

bool isLetter(char a) {
    if (int(a) >= 'a' && int(a) <= 'z' || int(a) >= 'A' && int(a) <= 'Z' || int(a) == '_')
        return true;
    else
        return false;
}

bool isDigit(char a) {
    if (int(a) >= '0' && int(a) <= '9')
        return true;
    else
        return false;
}

int Reserve(char* a) {
    for (int i=0; i < WORDLEN; i++) {
        if (!strcmp(a, reserveWord[i])) {
            return i;
        }
    }
    return -1;
}

char* int_char(int a) {
    char* b = new char[50];
    sprintf(b, "%d", a);
    return b;
}

char* InsertId(char* a) {
    char* b=new char[100];
    for (int i = 0; i < 1000; i++) {
        if (!strcmp(IDentifierTbl[i], "")) {
            strcpy(IDentifierTbl[i],a);
            b = int_char(i);
            return b;
        }
        else {
            if (!strcmp(IDentifierTbl[i], a)) {
                b = int_char(i);
                return b;
            }
        }
    }
    cout<<"ID表溢出！"<<endl;
        exit(0);
}

char* isInOpera(char* a,int& j) {
    char* temp = new char[3];
    for (int i = 0; i < ONEOPLEN; i++) {
        if (a[j]==oneOpera[i]) {
            temp[0] = a[j];
            temp[1] = a[j+1];
            temp[2] = '\0';
            j++;
            for (int k = 0; k < TWOOPLEN; k++) {
                if (!strcmp(temp, twoOpera[k])) {
                    j++;
                    return temp;
                }
            }
            temp[1] = '\0';
            return temp;
        }
    }
    temp[0] = '\0';
    return temp;;
}

char* InsertStrid(char* a) {
    char* b = new char[100];
    for (int i = 0; i < 1000; i++) {
        if (!strcmp(IDstr[i], "")) {
            strcpy(IDstr[i], a);
            b = int_char(i);
            return b;
        }
        else {
            if (!strcmp(IDstr[i], a)) {
                b = int_char(i);
                return b;
            }
        }
    }
}
//double a = 23.3e-100;
char* getDig(char* a, int& i,int &is_error) {
    char* b = new char[100];
    int bi = 0;
    while (isDigit(a[i])) {
        b[bi++] = a[i++];
    }
    if (a[i] == '.') {
        b[bi++] = a[i++];
        while (isDigit(a[i])) {
            b[bi++] = a[i++];
        }
    }
    if (a[i] == 'e' || a[i] == 'E') {
        b[bi++] = a[i++];
        if (a[i] == '-' || a[i] == '+') {
            b[bi++] = a[i++];
        }
        while (isDigit(a[i])) {
            b[bi++] = a[i++];
        }
    }
    b[bi] = '\0';
    if (isLetter(a[i])) {
        is_error = 1;
    }
    return b;
}

void deal_include(char* txt, char* value, char* id,int& ti,int& isInclude,char* a,int& is_error) {
    int ai = 0;
    if (isInclude == 1) {
        if (txt[ti] == '<'||txt[ti]=='"') {
            a[0] = txt[ti++];
            a[1] = '\0';
            strcpy(value, a);
            strcpy(id, "-");
            isInclude++;
        }
        else {
            is_error = 1;
        }
    }
    else if (isInclude == 2) {
        if (isLetter(txt[ti])) {
            while (isLetter(txt[ti]) || isDigit(txt[ti])||txt[ti]=='.') {//特别之处
                a[ai++] = txt[ti++];
            }
            a[ai] = '\0';
            strcpy(value, "id");
            strcpy(id, InsertId(a));
            isInclude++;
        }

    }
    else if (isInclude == 3) {
        if (txt[ti] == '>'||txt[ti] == '"') {
            a[0] = txt[ti++];
            a[1] = '\0';
            strcpy(value, a);
            strcpy(id, "-");
            isInclude = 0;
        }
        else {
            is_error = 1;
        }
    }
}

int one_fx(char* txt,char* value, char* id ,string& error_msg) {

    char* a=new char[100];
    int ai = 0;
    if (txt[ti] == ' '&&!isChar)
        ti++;
    if (isInclude) {
        int is_error=0;
        deal_include(txt, value, id, ti, isInclude,a,is_error);
        if (is_error) {
            error_msg = "there is an error near include";
            return 1;
        }
    }
    else if (isStr) {
        char as[1000];
        while (!(txt[ti] == '\"' && txt[ti - 1] != '\\'|| txt[ti] == '\"' && txt[ti - 1] == '\\' && txt[ti - 2] == '\\'||txt[ti]=='\0')) {
            as[ai++] = txt[ti++];
        }
        as[ai] = '\0';
        if (txt[ti] == '\0') {
            error_msg = "there is an error near ";
            error_msg += as;
            return 1;
        }
        cout << as << endl;
        strcpy(value, "strid");
        strcpy(id, InsertStrid(as));
        isStr = false;
    }
    else if (isChar==1) {
        if (txt[ti] == '\\') {
            a[ai++] = txt[ti++];
            a[ai++] = txt[ti++];
        }
        else {
            a[ai++] = txt[ti++];
        }
        a[ai] = '\0';
        strcpy(value, "id");
        strcpy(id, InsertId(a));
        isChar = 2;
        if (txt[ti] != '\'') {
            error_msg = "there is an error near ";
            error_msg += a;
            return 1;
        }
    }
    else if (isLetter(txt[ti])) {
        while (isLetter(txt[ti]) || isDigit(txt[ti])) {
            a[ai++] = txt[ti++];
        }
        a[ai] = '\0';
        int code = Reserve(a);
        if (code == -1) {
            strcpy(value, "id");
            strcpy(id, InsertId(a));
        }
        else {
            strcpy(value, a);
            strcpy(id, "-");
            if (!strcmp(value, "include")) {
                isInclude = 1;
            }
            if (!strcmp(value, "cout")) {
                int k = 1;
            }
        }
    }
    else if (isDigit(txt[ti])) {
        int is_error=0;
        a = getDig(txt, ti,is_error);
        if (is_error) {
            error_msg = "there is an error near ";
            error_msg += a;
            return 1;

        }
        strcpy(value, "digit");
        strcpy(id, a);
    }
    else if (txt[ti] == '\0') {
        strcpy(value, "\0");
    }
    else  {
        char* temp = isInOpera(txt, ti);
        if (temp[0] != '\0') {
            strcpy(value,temp);
            strcpy(id, "-");
            if (!strcmp(temp, "\"") && txt[ti - 2] != '\\'&&!isYinhao) {
                isStr = true;
                isYinhao = true;
            }
            else if ((!strcmp(temp, "\"") && txt[ti - 2] != '\\'|| !strcmp(temp, "\"") && txt[ti - 2] == '\\' && txt[ti - 3] == '\\') && isYinhao) {//注意a5 = "videotxt2\\"
                isYinhao = false;
            }
            else if (!strcmp(temp, "\'") && txt[ti - 2] != '\\' && isChar==0) {
                isChar = 1;
            }
            else if (!strcmp(temp, "\'") && txt[ti - 2] != '\\' && isChar==2) {
                isChar = 0;
            }
           }

        else {
            error_msg = "there is an error near ";
            error_msg += txt[ti];
            error_msg += '\0';
            return 1;
        }
    }
    return 0;
}

void changeToV(char* value, char* id, V* word, int& word_id)
{
    if (!strcmp(value, "id"))
    {
        word[word_id].name = "ID";
        word[word_id].realName = IDentifierTbl[stringToInt(id)];
    }
    else if (!strcmp(value, "strid"))
    {
        word[word_id].name = "strid";
        word[word_id].realName = IDstr[stringToInt(id)];
    }
    else if (!strcmp(value, "digit"))
    {
        word[word_id].name = "num";
        word[word_id].realName = id;
    }
    else if (!strcmp(value, "<") || !strcmp(value, "<=") || !strcmp(value, ">") || !strcmp(value, ">=") || !strcmp(value, "==") || !strcmp(value, "!="))
    {
        word[word_id].name = "relop";
        word[word_id].realName = value;
    }
    else
    {
        word[word_id].name = value;
        word[word_id].realName = "";
    }
    word[word_id].type = VT;
    word_id++;
}

int fenxi(char* txt, string& res1, string& res2, string& res3, string& error_msg, V* word, int& word_id) {
    int is_error = 0;
    while (1) {
        char* value = new char[100];
        char* id = new char[100];

        is_error = one_fx(txt, value, id, error_msg);
        if (is_error) {
            break;
        }
        if (!strcmp(value, "\0")) {
            break;
        }
        //cout << "<" << value << "," << id << ">\n";
        res1 = res1 + "<" + value + "," + id + ">\n";
        changeToV(value, id, word, word_id);
    }
    //cout << res1;
    int i = 0;
    while (strcmp(IDentifierTbl[i], ""))
    {
        char* temp = int_char(i);
        res2 = res2 + "{" + temp + "," + IDentifierTbl[i] + +"}\n";
        i++;
    }
    //cout << res2;
    //cout << "---";
    i = 0;
    while (strcmp(IDstr[i], ""))
    {
        char* temp = int_char(i);
        res3 = res3 + "{" + temp + "," + IDstr[i] + +"}\n";
        i++;
    }
    return is_error;
}
//头文件部分未解决
void lexical_analysis(char* txt, V* word, int& aTxt_len,int& is_error,string& res1,string& res2,string& res3,string& error_msg) {
    del_zs(txt);
    del_other(txt);
    //string res1, res2,res3, error_msg;
    is_error = fenxi(txt, res1, res2, res3,error_msg, word, aTxt_len);
    /*
    cout << len << endl << txt << endl;
    cout << "删除注释后：" << endl << txt << endl;
    cout << "删除整合其他后：" << endl << txt << endl;
    cout << "------------" << endl;
    cout << res1;
    cout << res2;
    cout << "------------" << endl;
    cout <<"是否错误："<< is_error << endl << error_msg;
    */
    return;
}





class syntactic_parser
{
private :
    string* txt;//语法规则对应的文本
    int txt_len;//语法规则对应的文本的长度
    STR* p;//所有的语法规则
    int p_len;//语法规则的长度
    V* Vt;//所有的终结符
    V* Vn;//所有的非终结符
    int Vt_len;//终结符的数量
    int Vn_len;//非终结符的数量
    FF* first;//所有非终结符对应的first集合
    int first_len;//first集合的长度
    Itemset* itemsets;//所有的项目集
    int itemset_len=0;//所有项目集的长度
    ALL_ACTION_GOTO* aag;//ACTION_GOTO表
    V* aTxt;//待分析的程序经过词法分析器分析后得到的结果
    int* state;//状态表
    //id均指向当前元素
    int state_id;//状态表现在所指的状态id
    //int step;//
    V* Symbol;//符号表
    int Symbol_id;//符号表现在所指的符号id
    int txt_id;//读取了文本的符号id
    FourTranslate* ft;//四元式队列
    int ft_len;//四元式的长度
    int tran_temp_id;//临时变量T的id
    stack<int> fillback;//记录需要回填的表达式的id
    stack<int> rememberback;//记录后面需要用的位置
    string* define_var;//记录定义的变量名
    int define_var_len;//定义的变量的长度

    bool is_In_Vt(V a);//是否是终结符
    bool is_In_Vn(V a);//是否是非终结符
    void Letter_First(V a);//计算a的first集合
    bool isInFirst(FF a, V b);//b是否在a的first集合中
    bool isInV(V* a, int a_len, V b);//b是否在集合a中
    int getVnIndex(V a);//a在非终结符表中的位置
    int getSTRIndex(STR a);//得到a在语法规则表中的位置
    void getOtherItem(Itemset& a, int a_i);//得到项目集中的其他项目
    void getOtherForward(Itemset& a, int a_i, int n_i);//得到其他的展望集合
    void changeOnce(int i);//第i个项目集拓展出其他的项目集
    int isInOtherItemset(Item a);//是否在其他已有的项目集中
    bool isItemSame(Item a, Item b);//2个项目是否相同
    void copyItem(Item orign, Item& target);//复制一份项目
    int getAction(int state,V a,int& action,int& target);//根据ACTION_GOTO表得到在当前状态，当前字符下应产生的动作
    string errorMsgChange(V a);//报错信息的V的转化
    void value_transmission(int target);//传递变量名或数值
    void get_ft_res();//四元式转化为string
    bool IDIsDefine(string dvar);//变量是否被定义
    void Translate(int& synIsError,string& synErrorMsg);//转化为四元式
    void showFT();//展示四元式
public:
    int aTxt_len;//词法分析器分析后得到的结果的长度
    int is_lex_error;//语法分析是否有错
    string lex_res, lex_variable, lex_string_table, lex_error_msg;//词法分析产生词法分析的结果、变量表、 字符串表、词法分析错误信息
    string syn_itemset, syn_action_goto, syn_process;//语法分析的产生的项目集、ACTION_GOTO表、语法分析的过程
    string ft_res;//四元式的字符串结果
    syntactic_parser();//建立的初始化
    void read_file(string path);//读取文件
    void getaTxt(char* txt);//调用词法分析器从文本中得到要分析的程序
    void get_Vt_Vn();//得到终结符和非终结符
    void get_First();//得到First集合
    void get_Itemset();//得到项目集
    void getInitItemset();//得到初始项目集
    void changeTogetOtherItem();//得到其他的项目集
    void createACTIONGO();//创建ACTION_GOTO表
    void Analysis(int& synIsError,string& synErrorMsg);//语法分析
    void init();//初始化一遍关键变量
};

syntactic_parser sp;

bool syntactic_parser::IDIsDefine(string dvar)
{
    for (int i = 0; i < define_var_len; i++)
    {
        if (dvar == define_var[i])
        {
            return true;
        }
    }
    return false;
}

void syntactic_parser::init()
{
    aTxt_len = 0;
    is_lex_error = 0;
    lex_variable = "id name\n";
    lex_string_table = "id string\n";
    lex_res = "";
    lex_error_msg = "";
    is_lex_error = 0;
    syn_process = "Id----State----Symbol----InputString\n";
    state = new int[MAX];
    state[0] = 0;
    state_id = 0;
    Symbol = new V[MAX];
    Symbol_id = -1;
    txt_id = 0;
    ft_len = 0;
    tran_temp_id = 0;
    ft_res = "";
    //delete define_var;
    define_var = new string[1000];
    define_var_len = 0;
}

syntactic_parser::syntactic_parser()
{
    txt = new string[TXT_MAX];
    p = new STR[MAX];
    p_len = 0;
    Vt = new V[MAX];
    Vt_len=0;
    Vn_len=0;
    Vn = new V[MAX];
    txt_len = 0;
    first = new FF[MAX];
    first_len = 0;
    itemsets = new Itemset[MAX];
    aTxt = new V[MAX];
    lex_variable += "id name\n";
    lex_string_table += "id string\n";
    syn_itemset += "expression index forward_char\n";
    syn_action_goto += "itemset_id  action\n";
    syn_process += "Id----State----Symbol----InputString\n";
    ft = new FourTranslate[MAX];
    state = new int[MAX];
    state[0] = 0;
    state_id = 0;
    Symbol = new V[MAX];
    Symbol_id = -1;
    txt_id = 0;
    ft_len = 0;
    tran_temp_id = 0;
    ft_res = "";
    define_var = new string[1000];
    define_var_len = 0;
}

void syntactic_parser::showFT()
{
    for (int i = 0; i < ft_len; i++)
    {
        cout<<ft[i].id<<": " << "(" << ft[i].one << "," << ft[i].two << "," << ft[i].three << "," << ft[i].four << ")" << endl;
    }
}
void syntactic_parser::read_file(string path)
{

    QString qpath = QString::fromStdString(path);
    QString applicationDirPath;
    applicationDirPath = QCoreApplication::applicationDirPath();
    qpath=applicationDirPath+"/"+qpath;
    QTextCodec *code = QTextCodec::codecForName("gb18030");
    if(code)
        path = code->fromUnicode(qpath).data();
    ifstream fin(path);
    cout<<path;
    txt_len = 0;
    while (!fin.eof())
    {
        fin>>txt[txt_len++];
        cout << txt[txt_len - 1] << endl;
    }
    cout << txt_len;
    fin.close();
}

void syntactic_parser::getaTxt(char* txt)
{
    //cout << "---------------------------------------------\n";
    //cout << "V" << endl;
    lexical_analysis(txt, aTxt, aTxt_len,is_lex_error,lex_res,lex_variable,lex_string_table,lex_error_msg);
    V temp;
    temp.name = "#";
    temp.type = VT;
    aTxt[aTxt_len++] = temp;
    for (int i = 0; i < aTxt_len; i++)
    {
        cout << aTxt[i].name << "," << aTxt[i].type << endl;
    }
/*
    ifstream fin(path);
    aTxt_len = 0;
    while (!fin.eof())
    {
        fin >> aTxt[aTxt_len].name;
        aTxt[aTxt_len++].type = VT;
    }
    V temp;
    temp.name = "#";
    temp.type = VT;
    aTxt[aTxt_len++] = temp;
    fin.close();
    */
}

bool syntactic_parser::is_In_Vn(V a)
{
    for (int i = 0; i < Vn_len; i++)
    {
        if (a.name == Vn[i].name&& a.type == Vn[i].type)
        {
            return true;
        }
    }
    return false;
}

bool syntactic_parser::is_In_Vt(V a)
{
    for (int i = 0; i < Vt_len; i++)
    {
        if (a.name == Vt[i].name && a.type == Vt[i].type)
        {
            return true;
        }
    }
    return false;
}

bool syntactic_parser::isInFirst(FF a,V b)
{
    for (int i = 0; i < a.first_list_len; i++)
    {
        if (a.first_list[i] == b)
        {
            return true;
        }
    }
    return false;
}

int syntactic_parser::getVnIndex(V a)
{
    for (int i = 0; i < Vn_len; i++)
    {
        if (Vn[i] == a)
        {
            return i;
        }
    }
    cout << "Vn out of index" << endl;
    exit(-1);
    return -1;
}

int syntactic_parser::getSTRIndex(STR a)
{

    for (int i = 0; i < p_len; i++)
    {
        bool flag = true;
        if (!(p[i].left == a.left)||p[i].right_len!=a.right_len)
        {
            continue;
        }
        for (int j = 0; j < p[i].right_len; j++)
        {
            if (!(isInV(a.right, a.right_len, p[i].right[j])))
            {
                flag=false;
                break;
            }
        }
        if (flag)
        {
            return i;
        }
    }
    return -1;
}

void syntactic_parser::get_Vt_Vn()
{
    int i = 0;
    for (int i = 0; i < txt_len; i++)
    {
        //左边
        string left;
        int j = 1;
        char temp[MAX];
        int ti = 0;
        while (txt[i][j]!='-')
        {
            temp[ti++] = txt[i][j++];
        }
        temp[ti] = '\0';
        p[p_len].left.name = temp;
        p[p_len].left.type = VN;
        if (!is_In_Vn(p[p_len].left))
        {
            Vn[Vn_len].name = temp;
            Vn[Vn_len++].type = VN;
        }
        //右边
        j+=2;
        while (txt[i][j] != '\0')
        {
            if (txt[i][j] == '@')
            {
                j++;
                ti = 0;
                while (txt[i][j] != '@'&& txt[i][j] != '$' && txt[i][j] != '\0')
                {
                    temp[ti++] = txt[i][j++];
                }
                temp[ti] = '\0';
                p[p_len].right[p[p_len].right_len].name = temp;
                p[p_len].right[p[p_len].right_len++].type = VT;
                if (!is_In_Vt(p[p_len].right[p[p_len].right_len-1]))
                {
                    Vt[Vt_len].name = temp;
                    Vt[Vt_len++].type = VT;
                }
            }
            else if (txt[i][j] == '$')
            {
                j++;
                ti = 0;
                while (txt[i][j] != '@' && txt[i][j] != '$' && txt[i][j] != '\0')
                {
                    temp[ti++] = txt[i][j++];
                }
                temp[ti] = '\0';
                p[p_len].right[p[p_len].right_len].name = temp;
                p[p_len].right[p[p_len].right_len++].type = VN;
                if (!is_In_Vn(p[p_len].right[p[p_len].right_len-1]))
                {
                    Vn[Vn_len].name= temp;
                    Vn[Vn_len++].type = VN;
                }
            }
        }
        p_len++;
    }
    //cout << "Vt:" << endl;
    //for (int i = 0; i < Vt_len; i++)
    //	cout << Vt[i].name << endl;
    //cout << "Vn:" << endl;
    //for (int i = 0; i < Vn_len; i++)
    //	cout << Vn[i].name << endl;
    /*for (int i = 0; i < p_len; i++)
    {
        cout << p[i].left.name << "->";
        for (int j = 0; j < p[i].right_len; j++)
        {
            cout << p[i].right[j].name <<  ",";
        }
        cout << endl;
    }*/
}

void syntactic_parser::get_First()
{
    for (int i = 0; i < Vn_len; i++)
    {
        //cout << "  " << Vn[i].name << "\t\t{";
        Letter_First(Vn[i]);
        /*for (int j = 0; j + 1 < first[i].first_list_len; j++)
        {
            cout << first[i].first_list[j].name << ",";
        }
        cout << first[i].first_list[first[i].first_list_len-1].name << "} " << endl;*/
    }
}

void syntactic_parser::Letter_First(V a)
{
    int firstIndex = getVnIndex(a);
        for (int i = 0; i < p_len; i++)                //第i个产生式
        {
            if (p[i].left == a)             //左侧字符是ch
            {
                if (p[i].right[0].name == "#")
                {
                    if (!isInFirst(first[firstIndex],p[i].right[0]))           //右侧第一个字符是空，并且#没有加入过集合，加入Vni的first集合中
                    {

                        first[firstIndex].first_list[first[firstIndex].first_list_len++] = p[i].right[0];
                    }
                }
                else if (p[i].right[0].type==VT)
                {
                    if (!isInFirst(first[firstIndex], p[i].right[0]))     //右侧第一个字符是Vt，加入Vni的first集合中
                    {
                        first[firstIndex].first_list[first[firstIndex].first_list_len++] = p[i].right[0];
                    }
                }
                else if (p[i].right[0].type == VN)                //右侧第一个字母是Vn
                {
                    if (p[i].right_len == 1)                      //只有一个字母
                    {
                        Letter_First( p[i].right[0]);                     //把右侧字母的first集合加入到左侧字母中
                        for (int k = 0; k < first[getVnIndex(p[i].right[0])].first_list_len; k++)
                        {
                            if (!isInFirst(first[firstIndex], first[getVnIndex(p[i].right[0])].first_list[k]))
                            {
                                first[firstIndex].first_list[first[firstIndex].first_list_len++] = first[getVnIndex(p[i].right[0])].first_list[k];
                            }
                        }
                    }
                    else                                               //多个字母都是Vn
                    {
                        for (int j = 0; j < p[i].right_len; j++)
                        {
                            Letter_First(p[i].right[j]);
                            V temp;
                            temp.name = "#";
                            temp.type = VT;
                            if (isInFirst(first[getVnIndex(p[i].right[j])],temp) && (j + 1) < p[i].right_len)             //右侧非末位字母的first集里有空
                            {
                                for (int t = 0; t < first[getVnIndex(p[i].right[j])].first_list_len; t++)
                                {
                                    if (!isInFirst(first[firstIndex], first[getVnIndex(p[i].right[j])].first_list[t])&& first[getVnIndex(p[i].right[j])].first_list[t].name!="#")       //右侧字母的无空first集加入到Vni的first集,是否加入
                                        first[firstIndex].first_list[first[firstIndex].first_list_len++] = first[getVnIndex(p[i].right[j])].first_list[t];
                                }
                                //cout << p[i].right[j]<<"already ";
                            }
                            else                                                                 //集合里无空或有空但是最后一个，直接加入
                            {
                                for (int t = 0; t < first[getVnIndex(p[i].right[j])].first_list_len; t++)
                                {
                                    if (!isInFirst(first[firstIndex], first[getVnIndex(p[i].right[j])].first_list[t]))
                                        first[firstIndex].first_list[first[firstIndex].first_list_len++] = first[getVnIndex(p[i].right[j])].first_list[t];
                                }
                                break;
                            }
                        }
                    }
                }
                else {
                    cout << "error";
                    exit(0);
                }
            }
        }
}

void syntactic_parser::get_Itemset()
{

}

void syntactic_parser::getInitItemset()
{
    V temp;
    temp.name = "#";
    temp.type = VT;
    for (int i = 0; i < p_len; i++)
    {
        if (p[i].left.name == "S")
        {
            itemsets[0].item[itemsets[0].item_len].expression = p[i];
            itemsets[0].item[itemsets[0].item_len].index = 0;
            itemsets[0].item[itemsets[0].item_len].forward[itemsets[0].item[itemsets[0].item_len].forward_len] = temp;
            itemsets[0].item[itemsets[0].item_len].forward_len++;
            itemsets[0].item_len++;
            break;
        }
    }
    getOtherItem(itemsets[0], 0);
    itemset_len++;

    syn_itemset+= "----------------------------------\n";
    for (int i = 0; i < itemsets[0].item_len; i++)
    {
        syn_itemset += itemsets[0].item[i].expression.left.name + "->";
        for (int j = 0; j < itemsets[0].item[i].expression.right_len; j++)
        {
            syn_itemset += itemsets[0].item[i].expression.right[j].name;
        }
        syn_itemset += itemsets[0].item[i].index + "  ";
        for (int k = 0; k < itemsets[0].item[i].forward_len; k++)
        {
            syn_itemset += itemsets[0].item[i].forward[k].name + " ";
        }
        syn_itemset += "\n";
    }
}

bool syntactic_parser::isInV(V* a, int a_len, V b)
{
    for (int i = 0; i < a_len; i++)
    {
        if (a[i] == b)
        {
            return true;
        }
    }
    return false;
}

void syntactic_parser::getOtherForward(Itemset& a, int a_i,int n_i)
{
    int i;
    V temp;
    temp.name = "#";
    temp.type = VT;
    for (i = a.item[a_i].index + 1; i < a.item[a_i].expression.right_len; i++)
    {
        if (is_In_Vt(a.item[a_i].expression.right[i]))
        {
            if (!isInV(a.item[n_i].forward, a.item[n_i].forward_len, a.item[a_i].expression.right[i]))
            {
                a.item[n_i].forward[a.item[n_i].forward_len++] = a.item[a_i].expression.right[i];
            }
            break;
        }
        else
        {
            int Vindex = getVnIndex(a.item[a_i].expression.right[i]);
            for (int j = 0; j < first[Vindex].first_list_len; j++)
            {
                if (!isInV(a.item[n_i].forward, a.item[n_i].forward_len, first[Vindex].first_list[j])&&!( first[Vindex].first_list[j]==temp))
                {
                    a.item[n_i].forward[a.item[n_i].forward_len++] = first[Vindex].first_list[j];
                }
            }
            if (!isInFirst(first[Vindex], temp))
            {
                break;
            }

        }
    }
    if (i == a.item[a_i].expression.right_len)
    {
        for (int j = 0; j < a.item[a_i].forward_len; j++)
        {
            if (!isInV(a.item[n_i].forward, a.item[n_i].forward_len, a.item[a_i].forward[j]))
            {
                a.item[n_i].forward[a.item[n_i].forward_len++] = a.item[a_i].forward[j];
            }
        }
    }
    return;
}

void syntactic_parser::getOtherItem(Itemset& a,int a_i)
{
    if (a.item[a_i].index >= a.item[a_i].expression.right_len)
    {
        return;
    }
    for (int i = 0; i < p_len; i++)
    {
        if (p[i].left == a.item[a_i].expression.right[a.item[a_i].index])
        {
            a.item[a.item_len].expression = p[i];
            a.item[a.item_len].forward;
            getOtherForward(a,a_i, a.item_len);
            a.item[a.item_len].index = 0;
            a.item_len++;
            getOtherItem(a, a.item_len - 1);
        }
    }
}

void syntactic_parser::copyItem(Item orign, Item& target)
{
    target.index = orign.index;
    target.forward_len = orign.forward_len;
    target.expression.left.name = orign.expression.left.name;
    target.expression.left.type = orign.expression.left.type;
    target.expression.right_len = orign.expression.right_len;
    target.expression.right_len = orign.expression.right_len;
    for (int i = 0; i < orign.forward_len; i++)
    {
        target.forward[i].name = orign.forward[i].name;
        target.forward[i].type = orign.forward[i].type;
    }
    for (int i = 0; i < orign.expression.right_len; i++)
    {
        target.expression.right[i].name = orign.expression.right[i].name;
        target.expression.right[i].type = orign.expression.right[i].type;
    }
}

bool syntactic_parser::isItemSame(Item a, Item b)
{
    if (!(a.expression.left == b.expression.left))
        return false;
    if ( a.index != b.index )
        return false;
    if ( a.forward_len != b.forward_len)
        return false;
    if (a.expression.right_len != b.expression.right_len)
        return false;
    for (int i = 0; i < a.expression.right_len; i++)
    {
        if (!(a.expression.right[i] == b.expression.right[i]))
        {
            return false;
        }
    }
    for (int i = 0; i < a.forward_len; i++)
    {
        if (!isInV(b.forward, b.forward_len, a.forward[i]))
            return false;
    }
    return true;
}

int syntactic_parser::isInOtherItemset(Item a)
{
    for (int i = 0; i < itemset_len; i++)
    {
        for (int j = 0; j < itemsets[i].item_len; j++)
        {
            if (isItemSame(itemsets[i].item[j],a))
            {
                return i;
            }
        }
    }
    return -1;
}

void syntactic_parser::changeOnce(int ai)
{
    for (int i = 0; i < itemsets[ai].item_len; i++)
    {
        if (itemsets[ai].item[i].index < itemsets[ai].item[i].expression.right_len)
        {
            Item temp;
            copyItem(itemsets[ai].item[i], temp);
            temp.index++;
            int id = isInOtherItemset(temp);
            V method ;
            method.type = itemsets[ai].item[i].expression.right[itemsets[ai].item[i].index].type;
            method.name = itemsets[ai].item[i].expression.right[itemsets[ai].item[i].index].name;

            if (id != -1)
            {
                itemsets[ai].change[itemsets[ai].change_len].target = id;
                itemsets[ai].change[itemsets[ai].change_len].method = method;
                itemsets[ai].change_len++;
            }
            else
            {
                copyItem(temp, itemsets[itemset_len].item[0]);
                itemsets[itemset_len].item_len++;
                getOtherItem(itemsets[itemset_len], 0);
                syn_itemset += "--------------"+intToString(itemset_len) +"--------------------\n";
                for (int i = 0; i < itemsets[itemset_len].item_len; i++)
                {
                    syn_itemset += itemsets[itemset_len].item[i].expression.left.name + "->";
                    for (int j = 0; j < itemsets[itemset_len].item[i].expression.right_len; j++)
                    {
                        syn_itemset += itemsets[itemset_len].item[i].expression.right[j].name;
                    }
                    syn_itemset += "  " + intToString(itemsets[itemset_len].item[i].index) + "  ";
                    for (int k = 0; k < itemsets[itemset_len].item[i].forward_len; k++)
                    {
                        syn_itemset+= itemsets[itemset_len].item[i].forward[k].name + " ";
                    }
                    syn_itemset += "\n";
                }

                itemsets[ai].change[itemsets[ai].change_len].target = itemset_len;
                itemset_len++;
                itemsets[ai].change[itemsets[ai].change_len].method = method;
                itemsets[ai].change_len++;
            }
        }
    }
}

void syntactic_parser::changeTogetOtherItem()
{
    for (int i = 0;i<itemset_len;i++)
    {
        changeOnce(i);
    }
}

void syntactic_parser::createACTIONGO()
{
    aag = new ALL_ACTION_GOTO[itemset_len];
    for (int i = 0; i < itemset_len; i++)
    {
        if (itemsets[i].change_len == 0)
        {
            ACTION_GOTO* ag = new ACTION_GOTO[itemsets[i].item[0].forward_len];
            for (int j = 0; j < itemsets[i].item[0].forward_len; j++)
            {
                ag[j].action = R;
                ag[j].method = itemsets[i].item[0].forward[j];
                ag[j].target = getSTRIndex(itemsets[i].item[0].expression);
                aag[i].len++;
            }
            aag[i].ag = ag;//?
        }
        else
        {
            ACTION_GOTO* ag = new ACTION_GOTO[itemsets[i].change_len];
            for (int j = 0; j < itemsets[i].change_len; j++)
            {
                if (itemsets[i].change[j].method.type == VT)
                {
                    ag[j].action = S;
                    ag[j].method = itemsets[i].change[j].method;
                    ag[j].target = itemsets[i].change[j].target;
                }
                else
                {
                    ag[j].action = GO;
                    ag[j].method = itemsets[i].change[j].method;
                    ag[j].target = itemsets[i].change[j].target;
                }
                aag[i].len++;
            }
            aag[i].ag = ag;
        }
    }
    syn_action_goto+= "----------------------------------\n";
    for (int i = 0; i < itemset_len; i++)
    {
        syn_action_goto+= intToString(i) + "  ";
        for (int j = 0; j < aag[i].len; j++)
        {
            if (aag[i].ag[j].action == S)
                syn_action_goto += "S";
            else if(aag[i].ag[j].action==R)
                syn_action_goto += "R";
            else if (aag[i].ag[j].action == GO)
                syn_action_goto += "G";
            syn_action_goto += "-" + aag[i].ag[j].method.name + "-" +intToString(aag[i].ag[j].target) + "  ";
        }
        syn_action_goto += "\n";
    }
}

int syntactic_parser::getAction(int state, V a, int& action, int& target)
{
    for (int i = 0; i < aag[state].len; i++)
    {
        if (aag[state].ag[i].method == a)
        {
            action = aag[state].ag[i].action;
            target = aag[state].ag[i].target;
            return 1;
        }
    }
    return 0;
}

string syntactic_parser::errorMsgChange(V a)
{
    if(a.name=="ID")
    {
        return a.realName;
    }
    else if(a.name=="strid")
    {
        return a.realName;
    }
    else if(a.name=="relop")
    {
        return a.realName;
    }
    else if(a.name=="num")
    {
        return a.realName;
    }
    else
    {
        return a.name;
    }
}

void syntactic_parser::value_transmission(int target)
{
    if (p[target].left.name == "IDnum" && p[target].right_len == 1)
    {
        state_id -= p[target].right_len;
        Symbol[Symbol_id].name = "IDnum";
        Symbol[Symbol_id].type = VN;
    }
    else if (p[target].left.name == "TFactor" && p[target].right[0].name != "~")
    {
        string temp = Symbol[Symbol_id].name;
        state_id -= p[target].right_len;
        Symbol_id -= p[target].right_len;
        Symbol[++Symbol_id] = p[target].left;
        Symbol[Symbol_id].realName = temp;
    }
    else if (p[target].left.name == "Factor" && p[target].right_len == 2)
    {
        string temp = Symbol[Symbol_id].realName;
        state_id -= p[target].right_len;
        Symbol_id -= p[target].right_len;
        Symbol[++Symbol_id] = p[target].left;
        Symbol[Symbol_id].realName = temp;
    }
    else if (p[target].left.name == "Factor" && p[target].right_len == 3 && p[target].right[0].name == "(" && p[target].right[1].name == "Expression" && p[target].right[2].name ==")" )
    {
        string temp = Symbol[Symbol_id-1].realName;
        state_id -= p[target].right_len;
        Symbol_id -= p[target].right_len;
        Symbol[++Symbol_id] = p[target].left;
        Symbol[Symbol_id].realName = temp;
    }
    else if (p[target].left.name == "Term" && p[target].right_len == 2 && p[target].right[0].name == "Factor" && p[target].right[1].name == "LoopFactor")
    {
        string temp = Symbol[Symbol_id].realName;
        state_id -= p[target].right_len;
        Symbol_id -= p[target].right_len;
        Symbol[++Symbol_id] = p[target].left;
        Symbol[Symbol_id].realName = temp;
    }
    else if (p[target].left.name == "LoopFactor" && p[target].right_len == 3 && p[target].right[1].name == "Factor" && p[target].right[2].name == "LoopFactor")
    {
        string temp = Symbol[Symbol_id].realName;
        state_id -= p[target].right_len;
        Symbol_id -= p[target].right_len;
        Symbol[++Symbol_id] = p[target].left;
        Symbol[Symbol_id].realName = temp;
    }
    else if ((p[target].left.name == "LoopTerm"|| p[target].left.name == "LoopFactor" || p[target].left.name == "LoopAddtion") && p[target].right_len == 1 && p[target].right[0].name == "~")
    {
        state_id -= p[target].right_len;
        Symbol[Symbol_id].name = p[target].left.name;
        Symbol[Symbol_id].type = VN;
        Symbol[Symbol_id].realName = Symbol[Symbol_id - 1].realName;
    }
    else if (p[target].left.name == "LoopTerm" && p[target].right_len == 3 && p[target].right[1].name == "Term" && p[target].right[2].name == "LoopTerm")
    {
        string temp = Symbol[Symbol_id].realName;
        state_id -= p[target].right_len;
        Symbol_id -= p[target].right_len;
        Symbol[++Symbol_id] = p[target].left;
        Symbol[Symbol_id].realName = temp;
    }
    else if (p[target].left.name == "Addtion" && p[target].right_len == 2 && p[target].right[0].name == "Term" && p[target].right[1].name == "LoopTerm")
    {
        string temp = Symbol[Symbol_id].realName;
        state_id -= p[target].right_len;
        Symbol_id -= p[target].right_len;
        Symbol[++Symbol_id] = p[target].left;
        Symbol[Symbol_id].realName = temp;
    }
    else if (p[target].left.name == "Expression" && p[target].right_len == 2 && p[target].right[0].name == "Addtion" && p[target].right[1].name == "LoopAddtion")
    {
        string temp = Symbol[Symbol_id].realName;
        state_id -= p[target].right_len;
        Symbol_id -= p[target].right_len;
        Symbol[++Symbol_id] = p[target].left;
        Symbol[Symbol_id].realName = temp;
    }
    else if (p[target].left.name == "LoopAddtion" && p[target].right_len == 3 && p[target].right[0].name=="relop" && p[target].right[1].name == "Addtion" && p[target].right[2].name == "LoopAddtion")
    {
        string temp = Symbol[Symbol_id].realName;
        state_id -= p[target].right_len;
        Symbol_id -= p[target].right_len;
        Symbol[++Symbol_id] = p[target].left;
        Symbol[Symbol_id].realName = temp;
    }
    else if (p[target].left.name == "AT"|| p[target].left.name == "ForT")
    {
        string temp = Symbol[Symbol_id].name;
        state_id -= p[target].right_len;
        Symbol[Symbol_id].name = p[target].left.name;
        Symbol[Symbol_id].type = VN;
        Symbol[Symbol_id].realName = temp;
    }
    else
    {
        state_id -= p[target].right_len;
        Symbol_id -= p[target].right_len;
        Symbol[++Symbol_id] = p[target].left;
    }
}

void syntactic_parser::Translate(int& synIsError,string& synErrorMsg)
{
    if (Symbol[Symbol_id].name == "Term" && Symbol[Symbol_id - 2].name == "Term" && (Symbol[Symbol_id - 1].name == "+" || Symbol[Symbol_id - 1].name == "-"))
    {
        ft[ft_len].id = ft_len;
        ft[ft_len].one = Symbol[Symbol_id - 1].name;
        ft[ft_len].two = Symbol[Symbol_id - 2].realName;
        ft[ft_len].three = Symbol[Symbol_id].realName;
        ft[ft_len].four = "T"+intToString(tran_temp_id);
        Symbol[Symbol_id].realName = ft[ft_len].four;
        ft_len++;
        tran_temp_id++;
    }
    else if (Symbol_id-1>=0&&Symbol[Symbol_id-1].name == "TFactor" && Symbol[Symbol_id].name == "IDnum" && Symbol[Symbol_id-1].realName!="")
    {
        ft[ft_len].id = ft_len;
        ft[ft_len].one = "@";
        ft[ft_len].two = Symbol[Symbol_id].realName;
        ft[ft_len].three = "_";
        ft[ft_len].four = "T" + intToString(tran_temp_id);
        Symbol[Symbol_id].realName = ft[ft_len].four;
        ft_len++;
        tran_temp_id++;
    }
    else if (Symbol_id-2>=0&&Symbol[Symbol_id].name == "Factor" && Symbol[Symbol_id - 2].name == "Factor" && (Symbol[Symbol_id - 1].name == "*" || Symbol[Symbol_id - 1].name == "/"))
    {
        ft[ft_len].id = ft_len;
        ft[ft_len].one = Symbol[Symbol_id - 1].name;
        ft[ft_len].two = Symbol[Symbol_id - 2].realName;
        ft[ft_len].three = Symbol[Symbol_id].realName;
        ft[ft_len].four = "T" + intToString(tran_temp_id);
        Symbol[Symbol_id].realName = ft[ft_len].four;
        ft_len++;
        tran_temp_id++;
    }
    else if (Symbol[Symbol_id].name == "Expression" && Symbol[Symbol_id - 1].name == "=" && (Symbol[Symbol_id - 3].name == "ID" || (Symbol[Symbol_id - 2].name == "ID")))
    {
        if (Symbol[Symbol_id - 2].name == "AT"|| Symbol[Symbol_id - 2].name == "ForT")
        {
            if (Symbol[Symbol_id - 2].realName != "~")
            {
                ft[ft_len].id = ft_len;
                ft[ft_len].one = Symbol[Symbol_id - 2].realName;
                ft[ft_len].two = Symbol[Symbol_id - 3].realName;
                ft[ft_len].three = Symbol[Symbol_id].realName;;
                ft[ft_len].four = "T" + intToString(tran_temp_id);
                Symbol[Symbol_id].realName = ft[ft_len].four;
                ft_len++;
                tran_temp_id++;
            }
            ft[ft_len].id = ft_len;
            ft[ft_len].one = ":=";
            ft[ft_len].two = Symbol[Symbol_id].realName;
            ft[ft_len].three = "_";
            ft[ft_len].four = Symbol[Symbol_id - 3].realName;
            Symbol[Symbol_id].realName = ft[ft_len].four;
            ft_len++;
        }
        else
        {
            ft[ft_len].id = ft_len;
            ft[ft_len].one = ":=";
            ft[ft_len].two = Symbol[Symbol_id].realName;
            ft[ft_len].three = "_";
            ft[ft_len].four = Symbol[Symbol_id - 2].realName;
            Symbol[Symbol_id].realName = ft[ft_len].four;
            ft_len++;
        }
    }
    else if (Symbol_id-2>=0&&Symbol[Symbol_id].name == "Addtion" && Symbol[Symbol_id - 1].name == "relop" && Symbol[Symbol_id - 2].name == "Addtion")
    {
        ft[ft_len].id = ft_len;
        ft[ft_len].one = "j" + Symbol[Symbol_id - 1].realName;
        ft[ft_len].two = Symbol[Symbol_id-2].realName;
        ft[ft_len].three = Symbol[Symbol_id].realName;
        ft[ft_len].four = intToString(ft_len+3);
        ft_len++;

        ft[ft_len].id = ft_len;
        ft[ft_len].one = ":=";
        ft[ft_len].two = "0";
        ft[ft_len].three = "_";
        ft[ft_len].four = "T" + intToString(tran_temp_id);
        ft_len++;

        ft[ft_len].id = ft_len;
        ft[ft_len].one = "j";
        ft[ft_len].two = "_";
        ft[ft_len].three = "_";
        ft[ft_len].four = intToString(ft_len + 2);
        ft_len++;

        ft[ft_len].id = ft_len;
        ft[ft_len].one = ":=";
        ft[ft_len].two = "1";
        ft[ft_len].three = "_";
        ft[ft_len].four = "T" + intToString(tran_temp_id);
        ft_len++;

        Symbol[Symbol_id].realName = "T" + intToString(tran_temp_id);
        tran_temp_id++;
    }
    else if (Symbol_id-2>=0&&Symbol[Symbol_id].name == "Expression" && Symbol[Symbol_id - 2].name == "if" )
    {
        ft[ft_len].id = ft_len;
        ft[ft_len].one = "jnz";
        ft[ft_len].two = Symbol[Symbol_id].realName;
        ft[ft_len].three = "_";
        ft[ft_len].four = intToString(ft_len+2);
        ft_len++;

        ft[ft_len].id = ft_len;
        ft[ft_len].one = "j";
        ft[ft_len].two = "_";
        ft[ft_len].three = "_";
        ft[ft_len].four = "0";
        fillback.push(ft_len);
        ft_len++;
    }
    else if (Symbol[Symbol_id].name == "ForStatement")
    {
        int fti = fillback.top();
        fillback.pop();
        ft[fti].four =intToString(ft_len+1);

        int rb = rememberback.top();
        rememberback.pop();
        ft[ft_len].id = ft_len;
        ft[ft_len].one = "j";
        ft[ft_len].two = "_";
        ft[ft_len].three = "_";
        ft[ft_len].four = intToString(rb);
        ft_len++;
    }
    else if (Symbol[Symbol_id].name == "else")
    {
        int fti=fillback.top();
        fillback.pop();

        ft[ft_len].id = ft_len;
        ft[ft_len].one = "j";
        ft[ft_len].two = "_";
        ft[ft_len].three = "_";
        ft[ft_len].four = "0";
        fillback.push(ft_len);
        ft_len++;
        ft[fti].four = intToString(ft_len);

    }
    else if (Symbol_id-4>=0&&Symbol[Symbol_id - 4].name == "else" && Symbol[Symbol_id - 1].name == "StatementString" && Symbol[Symbol_id - 2].name == "InternalDeclaration" && Symbol[Symbol_id].name == "}")
    {
    int fti = fillback.top();
    fillback.pop();
    ft[fti].four = intToString(ft_len);
    }
    else if (Symbol_id-5>=0&&Symbol[Symbol_id - 5].name == "if" && Symbol[Symbol_id - 1].name == "StatementBlock" && Symbol[Symbol_id - 3].name == "Expression" && Symbol[Symbol_id].name == "~")
    {
        int fti = fillback.top();
        fillback.pop();
        ft[fti].four = intToString(ft_len);
    }
    else if (Symbol_id-1>=0&&Symbol[Symbol_id].name == "(" && Symbol[Symbol_id - 1].name == "while")
    {
        rememberback.push(ft_len);
    }
    else if (Symbol[Symbol_id].name == "ForOne")
    {
        rememberback.push(ft_len);
    }
    else if (Symbol_id-1>=0&&Symbol[Symbol_id].name == "Expression"&& Symbol[Symbol_id - 1].name == "ForOne")
    {
    ft[ft_len].id = ft_len;
    ft[ft_len].one = "jnz";
    ft[ft_len].two = Symbol[Symbol_id].realName;
    ft[ft_len].three = "_";
    ft[ft_len].four = "0";
    ft_len++;

        ft[ft_len].id = ft_len;
        ft[ft_len].one = "j";
        ft[ft_len].two = "_";
        ft[ft_len].three = "_";
        ft[ft_len].four = "0";
        fillback.push(ft_len);
        fillback.push(ft_len - 1);
        ft_len++;
        rememberback.push(ft_len);
    }
    else if (Symbol[Symbol_id].name == "ForThree")
    {
        int r1 = rememberback.top();
        rememberback.pop();
        int r2 = rememberback.top();
        rememberback.pop();
        rememberback.push(r1);

        ft[ft_len].id = ft_len;
        ft[ft_len].one = "j";
        ft[ft_len].two = "_";
        ft[ft_len].three = "_";
        ft[ft_len].four = intToString(r2);
        ft_len++;

        int fti = fillback.top();
        fillback.pop();
        ft[fti].four = intToString(ft_len);
    }
    else if (Symbol_id-2>=0&&Symbol[Symbol_id].name == "Expression" && Symbol[Symbol_id - 2].name == "while")
    {
    ft[ft_len].id = ft_len;
    ft[ft_len].one = "jnz";
    ft[ft_len].two = Symbol[Symbol_id].realName;
    ft[ft_len].three = "_";
    ft[ft_len].four = intToString(ft_len + 2);
    ft_len++;

    ft[ft_len].id = ft_len;
    ft[ft_len].one = "j";
    ft[ft_len].two = "_";
    ft[ft_len].three = "_";
    ft[ft_len].four = "0";
    fillback.push(ft_len);
    ft_len++;

    }
    else if (Symbol[Symbol_id].name == "WhileStatement")
    {
        ft[ft_len].id = ft_len;
        ft[ft_len].one = "j";
        ft[ft_len].two = "_";
        ft[ft_len].three = "_";
        int rb = rememberback.top();
        rememberback.pop();
        ft[ft_len].four = intToString( rb );
        ft_len++;

        int fti = fillback.top();
        fillback.pop();
        ft[fti].four = intToString(ft_len);

    }
    else if (Symbol[Symbol_id].name == "ReturnStatement")
    {
    ft[ft_len].id = ft_len;
    ft[ft_len].one = "j";
    ft[ft_len].two = "_";
    ft[ft_len].three = "_";
    ft[ft_len].four = "dest_addr";
    ft_len++;
    }
    else if (Symbol_id-1>=0&&Symbol[Symbol_id].name == "Expression"&& Symbol[Symbol_id-1].name == "return")
    {
    ft[ft_len].id = ft_len;
    ft[ft_len].one = ":=";
    ft[ft_len].two = Symbol[Symbol_id].realName;
    ft[ft_len].three = "_";
    ft[ft_len].four = "eax";
    ft_len++;
    }
    else if (Symbol[Symbol_id-1].name == "keyword"&& Symbol[Symbol_id].name=="ID"&& Symbol[Symbol_id + 1].name!="(")
    {
        define_var[define_var_len++] = Symbol[Symbol_id].realName;
    }
    else if (Symbol_id-1>=0&&Symbol[Symbol_id].name == "ID"&& Symbol[Symbol_id].realName != "" && Symbol[Symbol_id - 1].name != "Type"&& Symbol[Symbol_id-1].name!="keyword")
    {
        if (!IDIsDefine(Symbol[Symbol_id].realName))
        {
        synErrorMsg= "There are variables("+Symbol[Symbol_id].realName+") that are not defined!";
        synIsError=1;
        return;
        }
    }
}

void syntactic_parser::get_ft_res()
{
    for (int i = 0; i < ft_len; i++)
    {
        ft_res+= intToString(ft[i].id) +": (" + ft[i].one + "," + ft[i].two + "," + ft[i].three + "," + ft[i].four + ")\n";
    }
}


void syntactic_parser::Analysis(int& synIsError,string& synErrorMsg)
{
    int action;
    int target;
    int bz = 0;
    syn_process+= "------------------------------------------\n";

    V kong;
    kong.name = "~";
    kong.type = VT;
    kong.realName = "";

    syn_process += intToString(bz) + "----";
    syn_process += intToString(state[0]) ;
    syn_process += "----";
    syn_process +=  " ";
    syn_process += "----";
    for (int i = txt_id; i < aTxt_len-1; i++)
    {
        syn_process += aTxt[i].name + ",";
    }
    syn_process += aTxt[aTxt_len - 1].name ;
    syn_process += "\n";
    while (1)
    {
        bz++;
        //cout<<syn_process<<endl;
        bool is_kong = false;
        int is_succ = getAction(state[state_id], aTxt[txt_id], action, target);
        if (!is_succ)
        {
            if (Symbol[Symbol_id].name == "return"&&aTxt[txt_id].name==";")
            {
                kong.name = "`";
            }
            else
            {
                kong.name = "~";
            }
            is_succ = getAction(state[state_id], kong, action, target);
            if (!is_succ)
            {
                synIsError=1;
                string error1,error2,error3;

                if(txt_id-1<0&&txt_id+1<aTxt_len)
                {
                    error1=errorMsgChange(aTxt[txt_id]);
                    error2=errorMsgChange(aTxt[txt_id+1]);
                    synErrorMsg="ERROR!Please check around "+error1+error2;
                }
                else if(txt_id+1>=aTxt_len)
                {
                    error1=errorMsgChange(aTxt[txt_id-2]);
                    error2=errorMsgChange(aTxt[txt_id-1]);
                    synErrorMsg="ERROR!Please check around "+error1+error2;
                }
                else
                {
                    error1=errorMsgChange(aTxt[txt_id-2]);
                    error2=errorMsgChange(aTxt[txt_id-1]);
                    error3=errorMsgChange(aTxt[txt_id]);
                    synErrorMsg="ERROR!Please check around "+error1+error2+error3;
                }
                return;
            }
            else
            {
                is_kong = true;
            }
        }

        if (target == 0 && action==R)
        {
            if (txt_id == aTxt_len - 1 )//&& state_id == 1
            {
                syn_process += "Success!\n";
                break;
            }
            else
            {
                synIsError=1;
                string error1,error2,error3;

                if(txt_id-1<0&&txt_id+1<aTxt_len)
                {
                    error1=errorMsgChange(aTxt[txt_id]);
                    error2=errorMsgChange(aTxt[txt_id+1]);
                    synErrorMsg="ERROR!Please check around "+error1+error2;
                }
                else if(txt_id+1>=aTxt_len)
                {
                    error1=errorMsgChange(aTxt[txt_id-2]);
                    error2=errorMsgChange(aTxt[txt_id-1]);
                    synErrorMsg="ERROR!Please check around "+error1+error2;
                }
                else
                {
                    error1=errorMsgChange(aTxt[txt_id-2]);
                    error2=errorMsgChange(aTxt[txt_id-1]);
                    error3=errorMsgChange(aTxt[txt_id]);
                    synErrorMsg="ERROR!Please check around "+error1+error2+error3;
                }
                return;
            }
        }
        if (action == S)
        {
            //Symbol[++Symbol_id] = aTxt[txt_id];
            state[++state_id] = target;
            if (!is_kong)
            {
                Symbol[++Symbol_id] = aTxt[txt_id];
                txt_id++;
            }
            else
            {
                Symbol[++Symbol_id] = kong;
            }
        }
        else if (action == R)
        {
            value_transmission(target);
            getAction(state[state_id], Symbol[Symbol_id], action, target);
            state[++state_id] = target;
        }
        Translate(synIsError,synErrorMsg);
        if(synIsError)
        {
            return;
        }
/*
        syn_process += intToString(bz) + "----";
        for (int i = 0; i < state_id; i++)
        {
            syn_process += intToString(state[i]) + ",";
        }
        syn_process += intToString(state[state_id]);
        syn_process += "----";
        for (int i = 0; i < Symbol_id; i++)
        {
            syn_process += Symbol[i].name + ",";
        }
        syn_process += Symbol[Symbol_id].name;
        syn_process += "----";
        for (int i = txt_id; i < aTxt_len - 1; i++)
        {
            syn_process += aTxt[i].name + ",";
        }
        syn_process += aTxt[aTxt_len - 1].name;
        syn_process += "\n";*/
        syn_process += intToString(bz) + "----";
        for (int i = 0; i < state_id; i++)
        {
            syn_process += intToString(state[i]) + ",";
        }
        syn_process += intToString(state[state_id]);
        syn_process += "----";
        for (int i = 0; i <= Symbol_id; i++)
        {
            if(Symbol[i].realName!="")
                syn_process += Symbol[i].name +"("+ Symbol[i] .realName+")" + ",";
            else
                syn_process += Symbol[i].name + ",";
        }
        //syn_process += Symbol[Symbol_id].name;
        syn_process += "----";
        for (int i = txt_id; i < aTxt_len - 1; i++)
        {
            syn_process += aTxt[i].name + ",";
        }
        syn_process += aTxt[aTxt_len - 1].name;
        syn_process += "\n";
    }
    get_ft_res();
    //showFT();
}

int fun_(string& lex_res,string& lex_variable,string& lex_string_table,string& syn_itemset,string& syn_action_goto,string& syn_process,string& sem_res,int& synIsError,string& synErrorMsg)
{

    sp.Analysis(synIsError,synErrorMsg);
    lex_res=sp.lex_res;
    lex_variable=sp.lex_variable;
    lex_string_table=sp.lex_string_table;
    syn_itemset=sp.syn_itemset;
    syn_action_goto=sp.syn_action_goto;
    syn_process=sp.syn_process;
    sem_res=sp.ft_res;
    /*cout << sp.lex_res << endl;
    cout << sp.lex_variable << endl;
    cout << sp.lex_string_table << endl;
    cout << sp.syn_itemset << endl;
    cout << sp.syn_action_goto << endl;
    cout << sp.syn_process << endl;*/
    return 0;
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    sp.read_file("grammar.txt");
    sp.get_Vt_Vn();
    sp.get_First();
    sp.getInitItemset();
    sp.changeTogetOtherItem();
    sp.createACTIONGO();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::paintEvent(QPaintEvent *event)//绘制背景
{

    QString applicationDirPath;
    applicationDirPath = QCoreApplication::applicationDirPath();
    QPainter painter(this);
    painter.drawPixmap(rect(),QPixmap(applicationDirPath+"/bj.png"),QRect());		//传入资源图片路径
}


void init()
{
    ti=0;
    isStr = false;
    isYinhao = false;
    isChar = 0;
    isInclude = 0;
    for(int i=0;i<IDLEN;i++)
    {
        if(strcmp(IDentifierTbl[i],""))
        {
            strcpy(IDentifierTbl[i],"");
        }
        else
        {
            break;
        }
    }
    for(int i=0;i<IDLEN;i++)
    {
        if(strcmp(IDstr[i],""))
        {
            strcpy(IDstr[i],"");
        }
        else
        {
            break;
        }
    }
    sp.init();

}
void MainWindow::on_pushButton_clicked()
{
    QString textstring =  ui->textEdit->toHtml();
    QString textPlianString =  ui->textEdit->toPlainText();
    QByteArray ba = textPlianString.toLatin1(); // must
    char* txt;
    txt=ba.data();
    string lex_res,lex_variable,lex_string_table,syn_itemset,syn_action_goto,syn_process,sem_res;
    int is_error;
    init();

    sp.getaTxt(txt);
    int synIsError=0;
    string synErrorMsg="";
    is_error=fun_(lex_res,lex_variable,lex_string_table,syn_itemset,syn_action_goto,syn_process,sem_res,synIsError,synErrorMsg);

    Qlex_res = QString(QString::fromLocal8Bit(lex_res.c_str()));

    Qlex_variable = QString(QString::fromLocal8Bit(lex_variable.c_str()));

    Qlex_string_table = QString(QString::fromLocal8Bit(lex_string_table.c_str()));

    Qsyn_itemset = QString(QString::fromLocal8Bit(syn_itemset.c_str()));

    Qsyn_action_goto = QString(QString::fromLocal8Bit(syn_action_goto.c_str()));

    Qsyn_process = QString(QString::fromLocal8Bit(syn_process.c_str()));

    Qsem_res= QString(QString::fromLocal8Bit(sem_res.c_str()));
    if(synIsError)
    {
    QString r3;
    r3 = QString(QString::fromLocal8Bit(synErrorMsg.c_str()));
    QString applicationDirPath;
    applicationDirPath = QCoreApplication::applicationDirPath();
    QMessageBox message(QMessageBox::NoIcon, "提示", r3);
    message.setIconPixmap(QPixmap(applicationDirPath+"/cuowu.jpg"));
    message.exec();									///---显示消息框
    }
    else
    {
        QString r3;
        r3 = "Success!";
        QString applicationDirPath;
        applicationDirPath = QCoreApplication::applicationDirPath();
        QMessageBox message(QMessageBox::NoIcon, "提示", r3);
        message.setIconPixmap(QPixmap(applicationDirPath+"/correct.jpg"));
        message.exec();									///---显示消息框
    }
}


void MainWindow::on_pushButton_2_clicked()
{
    QString fileName;
    QTextCodec *codec=QTextCodec::codecForName("UTF-8");
    fileName = QFileDialog::getOpenFileName(this,tr("文件"),"" ,tr( "text(*.txt)"));
    if( !fileName.isNull())
    {
    //fileName是选择的文件名
     QFile file(fileName) ;
    if( !file.open(QFile ::ReadOnly |QFile::Text))
    {
    QMessageBox::warning(this ,tr("Error" ) ,tr("read file error:&1" ).arg(file.errorString()));
    return;
    }
    QTextStream in(&file);
    in.setCodec("UTF-8");
    QString txt;
    txt=in.readAll();
    ui->textEdit->setText(txt);
}
}

void MainWindow::on_pushButton_3_clicked()
{
    dialog=new Dialog(this,Qlex_res,Qlex_variable,Qlex_string_table);
    dialog->setModal(false);
    dialog->show();
}

void MainWindow::on_pushButton_4_clicked()
{
    dialog=new Dialog2(this,Qsyn_itemset,Qsyn_action_goto,Qsyn_process);
    dialog->setModal(false);
    dialog->show();
}

void MainWindow::on_pushButton_5_clicked()
{
    dialog=new Dialog3(this,Qsem_res);
    dialog->setModal(false);
    dialog->show();
}
