#include <stdio.h>
#define TOKEN_LIMIT 101 // Token이 가질 수 있는 최대 길이


int linenum = 1; // Line Number 저장을 위한 변수
FILE *fp; // 처리중인 프로그램의 파일 포인터 변수
char nextch = '\n'; // Pushback 구현을 위해 미리 다음 글자를 저장하는 변수; 시작 라인 처리를 위해 \n로 초기화한다.

// 직관적인 Category 처리를 위한 Enumerator
typedef enum Category {
    End = -1, // End Of File
    Keyword,
    Identifier,
    IntegerLit,
    BooleanLit,
    FloatLit,
    CharLit,
    Operator,
    Separator,
    Error
} Category;

// Token이 가지는 err이 가지는 값을 좀더 직관적으로 처리하기 위한 Enumerator
typedef enum TokenError {
    UNKNOWN_OPERATOR = -10,
    UNEXPECTED_CHARACTER,
    UNKNOWN_CHARACTER ,
    UNCLOSED_CHARACTER_LITERAL,
    NON_ASCII_CHARACTER,
} TokenError;

// Token 자료구조를 위한 구조체
typedef struct Token {
    Category cat;               // 토큰의 타입을 위한 Category 변수
    char token[TOKEN_LIMIT];    // Token의 문자를 저장하는 변수
    int len;                    // Token의 길이를 저장하는 변수
    int err;                    // 오류가 났을 때의 Flag와 위치를 겸하는 변수
} Token;

// 무시해야 하는 문자인지 확인하는 함수
int isEOL(char c)
{
    return c==' '||c=='\n'||c=='\t'||c=='\r';
}

// ASCII 인지 확인하는 함수
int isAscii(char c)
{
    return c>=0 && c<=127;
}

// 문자인지 확인하는 함수
int isAlpha(char c)
{
    return c>='a'&&c<='z' || c>='A'&&c<='Z';
}

// 숫자인지 확인하는 함수
int isDigit(char c)
{
    return c>='0' && c<='9';
}

// Operator 문자에 해당하는지 확인하는 함수
int isOperator(char c)
{
    const int operlen=11;
    char opers[operlen] = "!*/+-<=>&|%";
    for(int i=0;i<operlen;i++)
    {
        if(opers[i]==c)
            return 1;
    }
    return 0;
}

// Operator 토큰인지 확인하는 함수
int isOperatorToken(Token t)
{
    const int operlen = 17;
    char * opers[operlen] = {"||","&&","==","!=","<=",">=","<",">","++","--","+","-","*","/","!","%","="}; // 길이가 긴것부터 확인
    for(int i=0;i<operlen;i++)
    {
        for(int j=0; opers[i][j]!='\0' && j<t.len; j++)
        {
            if(opers[i][j]==t.token[j])
            {

                if(opers[i][j+1]=='\0' && j+1==t.len)
                {
                    return 1;
                }
                continue;
            }
            break;
        }
    }
    return 0;
}

// Boolean Literal인지 확인하는 함수
int isBooleanToken(Token t)
{
    const int boollen = 2;
    char * bools[boollen] = {"true","false"};
    for(int i=0;i<boollen;i++)
    {
        for(int j=0; bools[i][j]!='\0' && j<t.len; j++)
        {
            if(bools[i][j]==t.token[j])
            {

                if(bools[i][j+1]=='\0' && j+1==t.len)
                {
                    return 1;
                }
                continue;
            }
            break;
        }
    }
    return 0;
}

// Separator인지 확인하는 함수
int isSeparator(char c)
{
    const int separlen = 8;
    char separs[separlen] = ";,(){}[]";
    for(int i=0;i<separlen;i++)
    {
        if(separs[i]==c)
            return 1;
    }
    return 0;
}

// Keyword인지 확인하는 함수
int isKeywordToken(Token t)
{
    const int keylen = 7;
    char * keys[keylen] = {"int","bool","float","char","if","for","while"};
    for(int i=0;i<keylen;i++)
    {
        for(int j=0; keys[i][j]!='\0' && j<t.len; j++)
        {
            if(keys[i][j]==t.token[j])
            {

                if(keys[i][j+1]=='\0' && j+1==t.len)
                {
                    return 1;
                }
                continue;
            }
            break;
        }
    }
    return 0;
}

// Identifier, Literal 그리고 Keyword의 제대로된 종료조건인지 확인하는 함수
int isEOT(char c)
{
    return isOperator(nextch)||isSeparator(nextch)||isEOL(nextch);
}

// Category를 String으로 반환해주는 함수
char* get_CatStr(Category c)
{
    switch (c)
    {
    case Keyword:
        return "Keyword ";
    case Identifier:
        return "Identifier ";
    case IntegerLit:
        return "IntegerLit ";
    case BooleanLit:
        return "BooleanLit ";
    case FloatLit:
        return "FloatLit ";
    case CharLit:
        return "CharLit ";
    case Operator:
        return "Operator ";
    case Separator:
        return "Separator ";
    case End:
        return "End of File";
    case Error:
        return "Error ";
    }
    return NULL;
}

// 토큰이 가진 오류를 문자열로 반환하는 함수
char* get_TokenErrorStr(Token t)
{
    switch (t.err)
    {
    case UNKNOWN_OPERATOR:
        return "UNKNOWN_OPERATOR";
    case UNEXPECTED_CHARACTER:
        return "UNEXPECTED_CHARACTOR";
    case UNKNOWN_CHARACTER:
        return "UNKNOWN_CHARACTER";
    case UNCLOSED_CHARACTER_LITERAL:
        return "UNCLOSED_CHARACTER_LITERAL";
    case NON_ASCII_CHARACTER:
        return "NON_ASCII_CHARACTER";
    case TOKEN_LIMIT+1:
        return "NO_ERROR";
    default:
        return "UNEXPECTED_CHARACTOR";
    }
}

// 다음 토큰을 반환하는 함수
Token get_Token()
{
    Token nextToken; nextToken.len=0; nextToken.cat=End; nextToken.err=TOKEN_LIMIT+1;
    while(1)
    {
        // 파일의 끝
        if(nextch == EOF)
        {
            break;
        }
        // 알파벳일 때 우선 Identifier라고 가정 후 문자 추가
        if(isAlpha(nextch))
        {
            if(nextToken.len==0)
            {
                nextToken.cat = Identifier;
            }
        }
        // 숫자일 때 첫 문자라면 Integer라고 가정 후 문자 추가
        // 만약 첫 문자가 아니라면 Identifier라고 가정
        else if(isDigit(nextch))
        {
            if(nextToken.len==0)
            {
                nextToken.cat = IntegerLit;
            }
        }
        // 다음 문자가 . 이고 현재 처리중인 토큰이 Integer Literal이라면 Float Literal로 카테고리 변경 후 문자 추가
        else if(nextch == '.' )
        {
            if(nextToken.cat == IntegerLit)
                nextToken.cat = FloatLit;
        }
        // 토큰이 \'로 시작하면 Character Literal로 가정 후 문자 추가
        else if(nextch=='\'')
        {
            if(nextToken.len == 0)
            {
                nextToken.cat = CharLit;
            }
        }
        // 줄바꿈이면 Line을 출력 후 linuenum에 1을 더해주고 continue를 사용해 무시한다
        else if(nextch == '\n')
        {
            int cur = ftell(fp);
            char str[201];
            fgets(str,200,fp);
            // 라인이 바뀌었음을 출력한다
            printf("Line %d: %s",linenum++,str);
            if(feof(fp))
                printf("\n");
            // 한줄 읽은 파일 포인터를 다시 전으로 돌려준다
            fseek(fp,cur-ftell(fp),SEEK_CUR);
            nextch = fgetc(fp);
            continue;
        }
        // 다음 문자가 Separator면 Separator로 가정하고 nextToken에 문자 추가
        else if(isSeparator(nextch))
        {
            nextToken.cat=Separator;
        }
        // 다음 문자가 Operator라면 Operator로 가정
        else if(isOperator(nextch))
        {
            if(nextToken.len == 0)
            {
                nextToken.cat=Operator;
            }
        }
        // 공백 무시 처리
        else if(nextch==' ' || nextch=='\t' || nextch=='\r')
        {
            nextch = fgetc(fp);
            continue;
        }
        // 다음 문자가 어떠한 State Transition으로도 해당하지 않을 때의 처리
        else
        {
            nextToken.cat=Error;
            nextToken.err=nextToken.len;
        }
        // 다음 문자를 토큰에 저장한다
        nextToken.token[nextToken.len] = nextch;
        nextToken.len++;

        // Pushback을 위해 다음 문자를 미리 읽는다.
        nextch = fgetc(fp);


        // 각 카테고리별로 종료조건을 확인해서 다음 문자를 계속 읽을지, nextToken을 반환할지 결정
        switch (nextToken.cat)
        {
        case Keyword:
        case BooleanLit:
        case Identifier:
            // EOT에 해당하면 제대로된 토큰을 반환
            if(isEOT(nextch))
            {
                // Identifier가 아니라 키워드중 하나인지 확인 후 카테고리 변경
                if(isKeywordToken(nextToken))
                {
                    nextToken.cat = Keyword;
                }
                // Boolean Literal인지 확인 후 카테고리 변경
                else if(isBooleanToken(nextToken))
                {
                    nextToken.cat = BooleanLit;
                }
                // Token이 Boolean literal인지 확인
                nextToken.token[nextToken.len] = '\0';
                return nextToken;
            }
            // 다음 문자가 Identifier에 해당하지 않는 문자이면 오류 처리
            else if(!isAlpha(nextch) && !isDigit(nextch))
            {
                nextToken.err = nextToken.len;
            }
            break;
        case IntegerLit:
            // EOT에 해당하면 제대로된 토큰을 반환
            if(isEOT(nextch))
            {
                nextToken.token[nextToken.len] = '\0';
                return nextToken;
            }
            // 다음 문자가 숫자나 .이 아니라면 오류 처리
            else if(!isDigit(nextch) && nextch!='.')
            {
                nextToken.err = nextToken.len;
            }
            break;

        case FloatLit:
            // EOT에 해당하면 제대로된 토큰을 반환
            if(isEOT(nextch))
            {
                nextToken.token[nextToken.len] = '\0';
                return nextToken;
            }
            // 다음 문자가 숫자가 아니라면 오류처리
            else if(!isDigit(nextch))
            {
                nextToken.err = nextToken.len;
            }
            break;
        case CharLit:
            // Character Literal의 경우 한개의 문자만 가지기 때문에 다음 문자가 ASCII인지 확인하고 \' 가 그 다음에 오는지 확인 후 종료
            if(isAscii(nextch))
            {
                nextToken.token[nextToken.len] = nextch;
                nextToken.len++;
                nextch = fgetc(fp);
                if(nextch == '\'')
                {
                    nextToken.token[nextToken.len] = nextch;
                    nextToken.len++;
                    nextch = fgetc(fp);
                    nextToken.token[nextToken.len] = '\0';
                    return nextToken;
                }
                // ASCII 문자 뒤로 \'로 닫히지 않았다면 오류발생
                else
                {
                    nextToken.err = UNCLOSED_CHARACTER_LITERAL; //Character 가 제대로 닫히지 않은 오류
                    nextToken.token[nextToken.len] = '\0';
                    return nextToken;
                }
            }
            // ASCII 문자가 오지 않았다면 오류발생
            else
            {
                nextToken.err = NON_ASCII_CHARACTER; // ASCII 문자가 오지 않은 오류
                nextToken.token[nextToken.len] = '\0';
                return nextToken;
            }
            break;
        case Separator:
            // Separator인 경우 한개씩 오기 때문에 바로 종료
            nextToken.token[nextToken.len] = '\0';
            return nextToken;
        case Operator:
            // 다음 문자가 Operator가 아닐 때의 종료조건 처리
            if(!isOperator(nextch))
            {
                if(isOperatorToken(nextToken))
                {
                    nextToken.token[nextToken.len] = '\0';
                    return nextToken;
                }
                // 존재하지 않는 형식의 오퍼레이터 오류 처리
                else
                {
                    nextToken.err = UNKNOWN_OPERATOR;
                    nextToken.token[nextToken.len] = '\0';
                    return nextToken;
                }
            }
            // Operator가 아닌 주석 // 인지 확인 후 처리
            else if(nextToken.len == 1 && nextToken.token[0]=='/' && nextch=='/')
            {
                char commentbuf[201];
                // 주석을 무시하기 위해 \n 까지 스킵한다
                fgets(commentbuf,200,fp);
                // 다음 라인의 첫번째 문자를 읽는다
                if(feof(fp))
                    nextch = fgetc(fp);
                else
                    nextch = '\n';
                // nextToken의 값들을 초기화해준다
                nextToken.cat=End;
                nextToken.len=0;
            }
            // 다음 문자가 Operator 일 때의 종료조건 처리
            else
            {
                nextToken.token[nextToken.len] = nextch;
                nextToken.len++;
                // 다음 문자를 추가해 줬을 때 Operator Token에 해당하는지 확인. 해당한다면 원래대로 돌려놓고 계속해서 다음 문자를 읽는다.
                if(isOperatorToken(nextToken))
                {
                    nextToken.len--;
                }
                // 다음 문자를 합쳤을 때 제대로 된 Operator Token이 아니라면 되돌려 놓고 토큰을 반환한다.
                else
                {
                    nextToken.len--;
                    nextToken.token[nextToken.len] = '\0';
                    return nextToken;
                }
            }
            break;
        
        // EOF 부분은 위에서 처리해 주므로 여기까지 오면 이상한 문자를 만났다고 볼 수 있다.
        case Error:
            nextToken.token[nextToken.len] = '\0';
                return nextToken;
        case End:
        default:
            break;
        }
    }
    return nextToken;
}

int main(int argc, char *argv[])
{
    char line[100];
    Token nextTok;
    // 파일 이름을 입력으로 받아 입력 파일을 연다
    
    // 커맨드라인 argument로 파일이름이 주어진 경우
    if(argc != 1)
    {
        fp = fopen(argv[1],"r");
    }
    // 그렇지 않은 경우 입력으로 받는다.
    else
    {
        printf("Name of input file: ");
        scanf("%s",line);
        fp = fopen(line,"r");
    }

    // EOF 까지 파일을 읽어 반환된 nextTok의 cat이 END일 때 까지 다음 토큰을 받아온다
    while((nextTok = get_Token()).cat != End)
    {
        printf("%s%s",get_CatStr(nextTok.cat),nextTok.token);
        // 오류가 발생했는지 확인하고 오류의 타입 출력
        if(nextTok.err!=TOKEN_LIMIT+1)
        {
            // State Transition에 없는 오류를 만난 경우
            if(nextTok.cat == Error)
            {
                printf(" Unknown Character %c",nextTok.token[nextTok.err]);
            }
            // Category에 해당하지 않는 문자를 만난 경우
            else if(nextTok.err >= 0)
            {
                printf(" got Unexpected Character %c",nextTok.token[nextTok.err]);
            }
            // Unclosed character literal, Unknown Operator등의 오류
            else
            {
                printf(" %s", get_TokenErrorStr(nextTok));
            }
        }
        printf("\n");
    }

    return 0;
}