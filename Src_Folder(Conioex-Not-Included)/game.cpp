//=============================================================================
// [game.cpp] ゲームシーン管理
// 
// 制作者：土居秀顕/花山大貴	制作日：2026/01/08
//=============================================================================
#include "main.h"
#include "game.h"
#include <iostream>
#include <fstream>
#include <chrono>

#define CONIOEX
#include "conioex.h"

// 定数・構造体
static int beatNum = 4;
static int frameCounter = 0;

//For Spawn Mode Switch
static bool g_spawnNormalNote = true;
static bool g_spawnLongNote = false;
static int switchNormalToLongTimer = 0;
static bool isLongNoteAppearCompletely = false;
static int switchLongToNormalTimer = 0;
static int readyToSpawnLongNoteTimer = 0;
bool activeNormalNotesStillExist;

enum SpawnMode
{
    MODE_NORMAL,
    MODE_CLEARING,
    MODE_LONG
};
static SpawnMode currentSpawnMode = MODE_NORMAL;
//For spawn mode test
static bool prevTabKey = false;

const int MAX_NOTES = 20;
//Normmal Note
struct Note
{
    int  x, y, oldY;
    bool active;
    bool  needsClear;
};
static Note notes[MAX_NOTES];
static int speedNormalNotesSpawn = 20;

//Long Note
enum LPathType { L_LINEAR, L_ZIGZAG, L_CURVE };
const int LNOTE_LEN = 20;
struct LongNote
{
    int x[LNOTE_LEN], y[LNOTE_LEN];
    int oldX[LNOTE_LEN], oldY[LNOTE_LEN];
    bool active;
    bool needsClear;
    bool isHolding;
    bool isHit[LNOTE_LEN];
    int startLaneX, endLaneX;
    LPathType pathType;
    int amplitude;
    int bgColor256[LNOTE_LEN];
};
static LongNote LNote;
static int speedLongNoteSpawn = 90;

//Judge Line
const int JUDGE_Y = 23;
const int LIMIT_Y = 26;
const int LANE_X[4] = { 4, 15, 26, 37 };

static int judgeColors[4] = { BLACK, BLACK, BLACK, BLACK };
static int judgeTimers[4] = { 0, 0, 0, 0 };
static bool prevKeys[4] = { false, false, false, false };

//ScoreBoard
const int DIGIT_PATTERNS[10][5][3] =
{
    {//--- 0 -----
        {1,1,1},
        {1,0,1},
        {1,0,1},
        {1,0,1},
        {1,1,1}
    },
    {//--- 1 -----
        {1,1,0},
        {0,1,0},
        {0,1,0},
        {0,1,0},
        {1,1,1}
    },
    {//--- 2 -----
        {1,1,1},
        {0,0,1},
        {1,1,1},
        {1,0,0},
        {1,1,1}
    },
    {//--- 3 -----
        {1,1,1},
        {0,0,1},
        {1,1,1},
        {0,0,1},
        {1,1,1}
    },
    {//--- 4 -----
        {1,0,1},
        {1,0,1},
        {1,1,1},
        {0,0,1},
        {0,0,1}
    },
    {//--- 5 -----
        {1,1,1},
        {1,0,0},
        {1,1,1},
        {0,0,1},
        {1,1,1}
    },
    {//--- 6 -----
        {1,1,1},
        {1,0,0},
        {1,1,1},
        {1,0,1},
        {1,1,1}
    },
    {//--- 7 -----
        {1,1,1},
        {1,0,1},
        {0,0,1},
        {0,0,1},
        {0,0,1}
    },
    {//--- 8 -----
        {1,1,1},
        {1,0,1},
        {1,1,1},
        {1,0,1},
        {1,1,1}
    },
    {//--- 8 -----
        {1,1,1},
        {1,0,1},
        {1,1,1},
        {0,0,1},
        {1,1,1}
    }
};
static int g_currentScore = 0;
static int g_oldScore = -1;
const int DIGIT_X = 52;
const int DIGIT_Y = 2;
//ScoreBoard --- Effect
const int BORDER_X1 = 50;const int BORDER_Y1 = 1;
const int BORDER_X2 = 80;const int BORDER_Y2 = 7;
const int NEON_RAMP_SIZE = 24;
const int NEON_TAIL = 24;
//ScoreBoard --- Effect --- Color

//BLACK_WHITE
static const int BLACK_WHITE_RAMP[] =
{
    232, 233, 234, 235, 236, 237, 238, 239, 240, 241, 242, 243,
    244, 245, 246, 247, 248, 249, 250, 251, 252, 253, 254, 255
};

//Combo ------- HeatBar
static const int HEAT_RAMP[] = { 196, 202, 208, 214, 220, 226 };
static const int HEAT_RAMP_SIZE = 6;

//Combo ------- HeatBar --- Rainbow color
static const int RAINBOW_RAMP[] =
{
    196, 202, 208, 214, 220, 226, 190, 154, 118, 82, 46,
    47, 48, 49, 50, 51, 45, 39, 33, 27, 21,
    57, 93, 129, 165, 201, 200, 199, 198, 197
};
static const int RAINBOW_RAMP_SIZE = 30;

//------------------------------------
//For Result Count
static int g_combo = 0;
static int g_combo_MAX = 0;
static int g_perfectCnt = 0;
static int g_greatCnt = 0;
static int g_missCnt = 0;
//------------------------------------
static int barCombo = 0;
static int BAR_COMBO_MAX = 100;
//------------------------------------
//Power Gauge
static int scoreGoal = 0;
static float gaugePer = 0.0f;
static int gaugeCount = 0;
static int lastGaugeCount = -1;
static int prevCount = -1;
const int batteryRamp[11] = { 196, 202, 208, 214, 220, 226, 190, 154, 118, 82, 46 };
const int GAUGE_MAX = 27;//27こスペースがマックス

//Scene
enum Scene{ HOME, SELECT, GAME, BONUS, RESULT };
int sceneFlag = 0;
enum Game { MUSIC_NORMAL, MUSIC_HARD, GUITAR_NORMAL, GUITAR_HARD };
int gameFlag = 0;

//Scene --- Home Scene
static LONG_PTR homeBgm;
char homeBgm_Path[] = "Future_1.mp3";
static LONG_PTR clickSound;
char clickSound_Path[] = "Click.mp3";
const int SCR_X1 = 1; const int SCR_Y1 = 1;
const int SCR_X2 = 80; const int SCR_Y2 = 25;
const int HOME_EFFECT_TAIL = 35;
static int startSceneData[25][80];
bool isStartSceneDrew = false;
static int lastHomeColorIdx = -1;

//Scene --- Select Scene
int musicIconFrame1[13][24];
int musicIconFrame2[13][24];
const int MUSIC_OFFSET_X = 11;
const int MUSIC_OFFSET_Y = 5;
bool musicFocused = false;
bool musicEffectDeleted = false;

const int MUSIC_X1 = 10; const int MUSIC_Y1 = 4;
const int MUSIC_X2 = 35; const int MUSIC_Y2 = 18;

int guitarIconFrame1[13][24];
int guitarIconFrame2[13][24];
const int GUITAR_OFFSET_X = 47;
const int GUITAR_OFFSET_Y = 5;
bool guitarFocused = false;
bool guitarEffectDeleted = false;

const int GUITAR_X1 = 46; const int GUITAR_Y1 = 4;
const int GUITAR_X2 = 71; const int GUITAR_Y2 = 18;

static bool lastMusicFocused = false;
static bool lastGuitarFocused = false;
static int lastMusicFrameIdx = -1;
static int lastGuitarFrameIdx = -1;

const int ICON_EFFECT_TAIL = 20;
static const int BLUE_RAMP[] = { 21, 27, 33, 39, 45, 51 };
static const int BLUE_RAMP_SIZE = 6;

//Select --- Difficulty
bool isMusicNormalFocused = false;
bool isMusicHardFocused = false;
bool isGuitarNormalFocused = false;
bool isGuitarHardFocused = false;

//Scene --- Game Scene
bool isStaticUIDrew = false;
bool isOutro = false;

static LONG_PTR gameMusic1;
char musicPath1[] = "maou_short_32_tokimeki.mp3";
static LONG_PTR gameMusic2;
char musicPath2[] = "maou_shining_star_short.mp3";
static LONG_PTR overDrum;
char overDrum_Path[] = "OverDrum.mp3";
static LONG_PTR notesHitSound;
char notesHitSound_Path[] = "NotesHit.mp3";

bool isMusicStartPlaying = false;
static std::chrono::steady_clock::time_point musicStartTime;
static float elapsedSec = 0.0f;//経過時間

//Scene --- Game Scene --- Emojis
static int emojisData[7][2][12][27];
const int EMOJI_OFFSET_X = 52;
const int EMOJI_OFFSET_Y = 13;
static int missCount = 0;
static int lastEmojiIdx = -1;
static int lastFrameIdx = -1;
static std::string precomputedEmojis[7][2][12];

//Scene --- Game Scene --- ゲーム終了
static int overHintData[9][43];
static bool readyToResult = false;
static bool isOverDrew = false;
static bool isOvering;
static int overTimeCounter = 0;

//Scene --- Result Scene
static LONG_PTR resultBgm;
char resultBgm_Path[] = "ResultMusic.mp3";
const enum Icon { DEFAULT, MUSIC, GUITAR };
static Icon iconSelected;
static int resultData[5][56];
static bool isResultDrew = false;
static int lastFrameIcon = -1;
static ULONGLONG resultEntryTime = 0;

//Scene --- Bonus
#define MAX_PATH_POINTS 200
const int CANVAS_X1 = 29;
const int CANVAS_Y1 = 8;
const int CANVAS_X2 = 51;
const int CANVAS_Y2 = 19;
const int TIME_LIMIT_MS = 30000;

struct GesturePoint
{
    int x;
    int y;
};

enum Shape { NONE, LINE_V, LINE_H, V_SHAPE, Z_SHAPE,
             M_SHAPE, W_SHAPE, INV_V_SHAPE, CIRCLE };

static LONG_PTR countDownSound;
char countDownSound_Path[] = "CountDown.mp3";
static LONG_PTR bonusBgm;
char bonusBgm_Path[] = "BonusTime.mp3";
static LONG_PTR bonusCorrect;
char bonusCorrect_Path[] = "Correct.mp3";
static LONG_PTR bonusWrong;
char bonusWrong_Path[] = "Wrong.mp3";

//BONUS SCENEの変数
static ULONGLONG startTime = 0;
static Shape currentTargetShape = NONE;
static Shape gestureShape = NONE;
static unsigned int g_seed = 0;
static bool reDrawTemplate = false;

//軌跡とテンプレートデータ
GesturePoint g_gesturePath[MAX_PATH_POINTS];
int g_pathSize = 0;
bool g_isWriting = false;
static char templateData[9][10][19];

//BONUS用の関数たち(宣言省略)
void ClearCanvas()
{
    const char* emptyRow = "\x1b[48;5;255m                       \x1b[0m";
    for (int y = CANVAS_Y1; y <= CANVAS_Y2; y++)
    {
        gotoxy(CANVAS_X1, y);
        std::cout << emptyRow;
    }
}
void LoadAllTemplates()
{
    const char* fnames[] =
    {
        "", "LINE_V.csv", "LINE_H.csv", "V_SHAPE.csv",
        "Z_SHAPE.csv", "M_SHAPE.csv", "W_SHAPE.csv",
        "INV_V.csv", "CIRCLE.csv"
    };

    for (int s = 1; s <= 8; s++)
    {
        std::ifstream file(fnames[s]);
        if (!file) continue;

        char ch;
        int row = 0, col = 0;

        while (file.get(ch) && row < 10)
        {
            if (ch == '1' || ch == '0')
            {
                templateData[s][row][col] = (ch == '1' ? 1 : 0);
                col++;
                if (col >= 19)
                {
                    col = 0; row++;
                }
            }
        }
        file.close();
    }
}
void DrawTemplate(Shape s)
{
    if (s <= NONE || s > CIRCLE) return;

    for (int y = 0; y < 10; y++)
    {
        gotoxy(31, 9 + y);
        std::string rowStr = "";
        for (int x = 0; x < 19; x++)
        {
            if (templateData[s][y][x] == 1)
            {
                rowStr += "\x1b[48;5;8m \x1b[0m";
            }
            else
            {
                rowStr += "\x1b[48;5;255m \x1b[0m";
            }
        }
        std::cout << rowStr;
    }
}
Shape AnalyzeGesture(GesturePoint* path, int size)
{
    if (size < 6) return NONE;

    int minX = 80, maxX = 0, minY = 25, maxY = 0;
    for (int i = 0; i < size; i++)
    {
        if (path[i].x < minX) minX = path[i].x;
        if (path[i].x > maxX) maxX = path[i].x;
        if (path[i].y < minY) minY = path[i].y;
        if (path[i].y > maxY) maxY = path[i].y;
    }

    int width = maxX - minX;
    int height = maxY - minY;
    int headTailDist = abs(path[0].x - path[size - 1].x) + abs(path[0].y - path[size - 1].y);

    //Gestures with no turns
    if (width > 6 && height <= 2) return LINE_H;
    if (height > 4 && width <= 2) return LINE_V;
    if (headTailDist < 6 && width > 5 && height > 3) return CIRCLE;

    //Gestures with turns
    int xTurns = 0; int yTurns = 0;
    int prevXDir = 0; int prevYDir = 0;
    int firstYDir = 0;

    for (int i = 1; i < size; i++)
    {
        //X Axis
        int dx = path[i].x - path[i - 1].x;
        int currentXDir = (dx > 0) ? 1 : (dx < 0 ? -1 : 0);
        if (currentXDir != 0)
        {
            //Record Y turn
            if (prevXDir != 0 && currentXDir != prevXDir) xTurns++;
            prevXDir = currentXDir;
        }

        //Y Axis
        int dy = path[i].y - path[i - 1].y;
        int currentYDir = (dy > 0) ? 1 : (dy < 0 ? -1 : 0);
        if (currentYDir != 0)
        {
            //Record the first Y direction
            if (firstYDir == 0) firstYDir = currentYDir;
            //Record Y turn
            if (prevYDir != 0 && currentYDir != prevYDir) yTurns++;
            prevYDir = currentYDir;
        }

    }

    if (yTurns >= 3) return (firstYDir == -1) ? M_SHAPE : W_SHAPE;

    if (xTurns >= 2 && width > 8)
    {
        if (path[size - 1].x > path[0].x && height > 2) return Z_SHAPE;
    }

    if (yTurns == 1)
    {
        return (firstYDir == 1) ? V_SHAPE : INV_V_SHAPE;
    }

    if (yTurns == 2 && width > 7) return Z_SHAPE;

    return NONE;
}
void DrawBonusEffect(int length, bool isToWhite)
{
    int rampSize = sizeof(BLACK_WHITE_RAMP) / sizeof(BLACK_WHITE_RAMP[0]);

    for (int i = 0; i < length; i++)
    {
        int index = i * rampSize / length;
        if (index >= rampSize) index = rampSize - 1;
        int colorIndex = isToWhite ? index : (rampSize - 1 - index);
        std::cout << "\x1b[48;5;" << BLACK_WHITE_RAMP[colorIndex] << "m ";
    }
    std::cout << "\x1b[0m";
}
bool canvasEffectDrew = false;

//BONUS TIME COUNTDOWN
static int countdownData[4][12][80];
static bool isCountingDown = true;
static int currentCountdownIdx = -1;
static int lastPlayedIdx = -1;
static bool bonusBgmPlayed = false;
void LoadCountDowns()
{
    const char* fnames[] =
    {
        "CountDown_4.csv", "CountDown_3.csv",
        "CountDown_2.csv", "CountDown_1.csv"
    };

    for (int s = 0; s < 4; s++)
    {
        std::ifstream file(fnames[s]);
        if (!file) continue;

        char ch;
        int row = 0, col = 0;

        while (file.get(ch) && row < 12)
        {
            if (ch >= '0' && ch <= '5')
            {
                countdownData[s][row][col] = ch - '0';
                col++;
                if (col >= 80)
                {
                    col = 0; row++;
                }
            }
        }
        file.close();
    }
}
void DrawCountDowns(int cnt)
{
    for (int y = 0; y < 12; y++)
    {
        gotoxy(1, 8 + y);
        for (int x = 0; x < 80; x++)
        {
            int val = countdownData[cnt][y][x];

            if (val == 1)      std::cout << "\x1b[48;5;255m ";
            else if (val == 2) std::cout << "\x1b[48;5;16m ";
            else if (val == 3) std::cout << "\x1b[48;5;1m ";
            else if (val == 4) std::cout << "\x1b[48;5;3m ";
            else if (val == 5) std::cout << "\x1b[48;5;2m ";
        }
    }
    std::cout << "\x1b[0m";
}
static int lastFillWidth = -1;
void DrawCountdownBar(int current, int max, int width, int x, int y)
{
    if (max <= 0) return;

    int fillWidth = (current * width) / max;
    if (fillWidth < 0) fillWidth = 0;
    if (fillWidth > width) fillWidth = width;

    if (fillWidth == lastFillWidth)
    {
        return;
    }
    lastFillWidth = fillWidth;

    gotoxy(x, y);
    for (int i = 0; i < width; i++)
    {
        if (i < fillWidth)
        {
            int colorIdx = (fillWidth * 10) / width;
            std::cout << "\x1b[48;5;" << batteryRamp[colorIdx] << "m ";
        }
        else
        {
            std::cout << "\x1b[48;5;16m ";
        }
    }

    std::cout << "\x1b[0m" << std::flush;
}

//----------My Methods-----------
//Scene --- Home
void LoadStartScene();
void DrawStartScene();
void DrawHomeEffect();
//Scene --- Select
void LoadMusicFrame1();
void LoadMusicFrame2();
void LoadGuitarFrame1();
void LoadGuitarFrame2();

void DrawMusicFrame1(int offset_x, int offset_y);
void DrawMusicFrame2(int offset_x, int offset_y);
void DrawGuitarFrame1(int offset_x, int offset_y);
void DrawGuitarFrame2(int offset_x, int offset_y);

void MusicFocusedEffect();
void ClearMusicFocusedEffect();
void GuitarFocusedEffect();
void ClearGuitarFocusedEffect();
//Scene --- Game
//Notes
void UpdateNormalNotes();
void DrawNormalNotes();
//LongNotes
void UpdateLongNote(bool isPress);
void DrawLongNote();
//ScoreBoard
void DrawLargeDigit(int startX, int startY, int digit);
void DrawScoreBoard(int x, int y, long score);
//NeonEffect
void DrawScoreNeonEffect();
void GetNeonXY(int pos, int X_LEFT, int Y_TOP, int X_RIGHT, int Y_BOTTOM, int& tx, int& ty);
int GetPathSize(int x1, int y1, int x2, int y2);
//Combo -> HeatBar
void DrawHeatBar();
//PowerGauge
void DrawPowerGauge();
//Emoji
void LoadEmojis();
void DrawEmojis();
//OverHint
void LoadOverHint();
void DrawOverHint();

//Scene --- Game
//Result
void LoadResult();
void DrawResult();

//Reset
void ResetGameVariables()
{
    //-------------------
    g_currentScore = 0;
    g_oldScore = -1;
    g_combo = 0;
    g_combo_MAX = 0;
    g_perfectCnt = 0;
    g_greatCnt = 0;
    g_missCnt = 0;
    missCount = 0;
    barCombo = -1;
    
    //-------------------
    gaugePer = 0.0f;
    gaugeCount = 0;
    lastGaugeCount = -1;
    lastEmojiIdx = -1;
    lastFrameIdx = -1;
    isStaticUIDrew = false;
    lastFillWidth = -1;

    //-------------------
    isOutro = false;
    isOvering = false;
    isOverDrew = false;
    overTimeCounter = 0;
    readyToResult = false;
    isMusicStartPlaying = false;
    elapsedSec = 0.0f;
    speedNormalNotesSpawn = 20;

    //-------------------
    currentSpawnMode = MODE_NORMAL;
    g_spawnNormalNote = true;
    g_spawnLongNote = false;
    switchNormalToLongTimer = 0;
    switchLongToNormalTimer = 0;
    readyToSpawnLongNoteTimer = 0;
    isLongNoteAppearCompletely = false;
    activeNormalNotesStillExist = false;

    //-------------------
    for (int i = 0; i < MAX_NOTES; i++)
    {
        notes[i].active = false;
        notes[i].needsClear = false;
        notes[i].y = 0;
        notes[i].oldY = 0;
    }
    LNote.active = false;
    LNote.needsClear = false;
    LNote.isHolding = false;
    for (int i = 0; i < LNOTE_LEN; i++) LNote.isHit[i] = false;

    //-------------------
    for (int i = 0; i < 4; i++)
    {
        prevKeys[i] = false;
        judgeTimers[i] = 0;
        judgeColors[i] = BLACK;
    }

    //-------------------
    //Bonus
    startTime = GetTickCount64();
    isCountingDown = true;
    currentCountdownIdx = -1;
    lastPlayedIdx = -1;
    bonusBgmPlayed = false;
    canvasEffectDrew = false;
    reDrawTemplate = true;
    g_isWriting = false;
    g_pathSize = 0;
    currentTargetShape = NONE;
    gestureShape = NONE;
}

// =============================================================================
// ゲームシーン初期化
// =============================================================================
void InitializeGame(void)
{
    //----------------- Console Windows Setting Change ----------------------------------------------------------------
    std::ios::sync_with_stdio(false);
    std::cin.tie(NULL);
    const HANDLE hStdin = GetStdHandle(STD_INPUT_HANDLE);
    DWORD dwMode_default;
    GetConsoleMode(hStdin, &dwMode_default);//終了時にデフォルト状態に戻すため、デフォルトの情報を保持
    SetConsoleMode(hStdin, ENABLE_WINDOW_INPUT | ENABLE_MOUSE_INPUT | ENABLE_EXTENDED_FLAGS);//マウス入力可否設定

    //Change Font
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_FONT_INFOEX cfi;
    cfi.cbSize = sizeof(cfi);
    GetCurrentConsoleFontEx(hOut, FALSE, &cfi);
    cfi.FontWeight = FW_BOLD;
    wcscpy_s(cfi.FaceName, L"MS Gothic");
    SetCurrentConsoleFontEx(hOut, FALSE, &cfi);
    //-----------------------------------------------------------------------------------------------------------------
    srand((unsigned int)time(NULL));
    frameCounter = 0;

    //Scene --- Start Scene
    LoadStartScene();

    //Scene --- Select Scene
    LoadMusicFrame1();
    LoadMusicFrame2();
    LoadGuitarFrame1();
    LoadGuitarFrame2();

    //Scene --- GAME
    LoadEmojis();
    LoadOverHint();

    //Scene --- Result
    LoadResult();

    for (int i = 0; i < MAX_NOTES; i++)
    {
        notes[i].active = false;
        notes[i].needsClear = false;
    }

    gameMusic1 = opensound(musicPath1);
    gameMusic2 = opensound(musicPath2);

    //Scene --- Result
    iconSelected = DEFAULT;

    //Scene --- Bonus
    LoadAllTemplates();
    LoadCountDowns();
    g_pathSize = 0;
    g_isWriting = false;

    //All Sounds
    homeBgm = opensound(homeBgm_Path);
    playsound(homeBgm, 1);
    overDrum = opensound(overDrum_Path);
    notesHitSound = opensound(notesHitSound_Path);
    countDownSound = opensound(countDownSound_Path);
    bonusBgm = opensound(bonusBgm_Path);
    bonusCorrect = opensound(bonusCorrect_Path);
    bonusWrong = opensound(bonusWrong_Path);
    resultBgm = opensound(resultBgm_Path);
    clickSound = opensound(clickSound_Path);
    
}

// =============================================================================
// ゲームシーン更新
// =============================================================================
void UpdateGame(void)
{
    frameCounter++;
    static bool prevLeftPressed = false;
    bool isLeftPressed = inport(PM_LEFT);

    if (sceneFlag == HOME)
    {
        if (isLeftPressed && !prevLeftPressed)
        {
            playsound(clickSound, 0);
            sceneFlag = SELECT;
            isStartSceneDrew = false;
            clrscr();

            lastMusicFrameIdx = -1;
            lastGuitarFrameIdx = -1;
            lastMusicFocused = !musicFocused;
            lastGuitarFocused = !guitarFocused;

            ResetGameVariables();

            //Test
            //g_currentScore = 0;
            //g_combo_MAX = 150;
            //g_perfectCnt = 999;
            //g_greatCnt = 999;
            //g_missCnt = 999;
            //iconSelected = MUSIC;
            //startTime = GetTickCount64();
            //g_seed = (unsigned int)time(NULL);
            //currentTargetShape = (Shape)((g_seed % 8) + 1);
            //g_pathSize = 0;
            //isCountingDown = true;
        }
    }
    else if (sceneFlag == SELECT)
    {
        int mouseX_SELECT = inport(PM_CURX);
        int mouseY_SELECT = inport(PM_CURY);

#ifdef _DEBUG
        gotoxy(1, 2);
        std::cout << "X: " << mouseX_SELECT << " Y: " << mouseY_SELECT;
#endif

        //---------------------------------------------------------------
        //MusicIcon
        if ((mouseX_SELECT > MUSIC_X1 && mouseX_SELECT < MUSIC_X2) &&
            (mouseY_SELECT > MUSIC_Y1 && mouseY_SELECT < MUSIC_Y2))//Icon
        {
            musicFocused = true;
        }
        else musicFocused = false;

        //MusicDifficulty --- NORMAL
        if ((mouseX_SELECT >= 17 && mouseX_SELECT <= 27) &&
            (mouseY_SELECT >= 19 && mouseY_SELECT <= 21))//Button
        {
            isMusicNormalFocused = true;
            musicFocused = true;
        }
        else
        {
            isMusicNormalFocused = false;
        }

        //MusicDifficulty --- HARD
        if ((mouseX_SELECT >= 17 && mouseX_SELECT <= 27) &&
            (mouseY_SELECT >= 22 && mouseY_SELECT <= 24))//Button
        {
            isMusicHardFocused = true;
            musicFocused = true;
        }
        else
        {
            isMusicHardFocused = false;
        }
        //---------------------------------------------------------------

        //---------------------------------------------------------------
        //GuitarIcon
        if ((mouseX_SELECT > GUITAR_X1 && mouseX_SELECT < GUITAR_X2) &&
            (mouseY_SELECT > GUITAR_Y1 && mouseY_SELECT < GUITAR_Y2))//Icon
        {
            guitarFocused = true;
        }
        else guitarFocused = false;

        //GuitarDifficulty --- NORMAL
        if ((mouseX_SELECT >= 53 && mouseX_SELECT <= 63) &&
            (mouseY_SELECT >= 19 && mouseY_SELECT <= 21))//Button
        {
            isGuitarNormalFocused = true;
            guitarFocused = true;
        }
        else
        {
            isGuitarNormalFocused = false;
        }

        //GuitarDifficulty --- HARD
        if ((mouseX_SELECT >= 53 && mouseX_SELECT <= 63) &&
            (mouseY_SELECT >= 22 && mouseY_SELECT <= 24))//Button
        {
            isGuitarHardFocused = true;
            guitarFocused = true;
        }
        else
        {
            isGuitarHardFocused = false;
        }
        //---------------------------------------------------------------

        //Scene Change
        if (inport(PM_LEFT) != 0)
        {
            if (isMusicNormalFocused)
            {
                sceneFlag = GAME;
                scoreGoal = 200000;
                iconSelected = MUSIC;
                gameFlag = MUSIC_NORMAL;
                playsound(clickSound, 0);
                stopsound(homeBgm);
                clrscr();
            }
            else if (isMusicHardFocused)
            {
                sceneFlag = GAME;
                scoreGoal = 500000;
                iconSelected = MUSIC;
                gameFlag = MUSIC_HARD;
                playsound(clickSound, 0);
                stopsound(homeBgm);
                clrscr();
            }
            else if (isGuitarNormalFocused)
            {
                sceneFlag = GAME;
                scoreGoal = 200000;
                iconSelected = GUITAR;
                gameFlag = GUITAR_NORMAL;
                playsound(clickSound, 0);
                stopsound(homeBgm);
                clrscr();
            }
            else if (isGuitarHardFocused)
            {
                sceneFlag = GAME;
                scoreGoal = 500000;
                iconSelected = GUITAR;
                gameFlag = GUITAR_HARD;
                playsound(clickSound, 0);
                stopsound(homeBgm);
                clrscr();
            }
        }
    }
    else if (sceneFlag == GAME)
    {
        //終了段階
        if (readyToResult == true)
        {
            if (g_currentScore >= scoreGoal)
            {
                sceneFlag = BONUS;
                startTime = GetTickCount64();
                g_seed = (unsigned int)time(NULL);
                currentTargetShape = (Shape)((g_seed % 8) + 1);
                g_pathSize = 0;
            }
            else
            {
                sceneFlag = RESULT;
                playsound(resultBgm, 0);
                resultEntryTime = GetTickCount64();
            }

            clrscr();
            lastMusicFrameIdx = -1;
            lastGuitarFrameIdx = -1;
            isResultDrew = false;
            readyToResult = false;
            return;
        }

        //---------------------------------------------------
        //TestDebug TAB change bool
        /*bool currentTab = (bool)inport(PK_TAB);
        if (currentTab && !prevTabKey)
        {
            g_spawnNormalNote = !g_spawnNormalNote;
            g_spawnLongNote = !g_spawnLongNote;
        }
        prevTabKey = currentTab;*/
        //---------------------------------------------------

        //---------------------------------------------------
        //本番
        if (isOutro == false)
        {
            switch (currentSpawnMode)
            {
            case MODE_NORMAL:
                g_spawnNormalNote = true;
                g_spawnLongNote = false;

                if (barCombo >= 10)
                {
                    switchNormalToLongTimer++;
                    if (switchNormalToLongTimer >= 300)
                    {
                        g_spawnNormalNote = false;//もう新しく生成しない
                        currentSpawnMode = MODE_CLEARING;

                        switchNormalToLongTimer = 0;
                    }
                }
                break;

            case MODE_CLEARING:
                //生成したノーマル全部出てきたかチェックする
                activeNormalNotesStillExist = false;
                for (int i = 0; i < MAX_NOTES; i++)
                {
                    if (notes[i].active && notes[i].y <= 1)
                    {
                        activeNormalNotesStillExist = true;
                        break;
                    }
                }

                if (!activeNormalNotesStillExist)//もうなかったようでロング生成していい
                {
                    if (++readyToSpawnLongNoteTimer > 15)
                    {
                        g_spawnLongNote = true;
                        currentSpawnMode = MODE_LONG;
                        readyToSpawnLongNoteTimer = 0;
                    }
                }
                break;

            case MODE_LONG:
                if (LNote.active && LNote.y[LNOTE_LEN - 1] == 1)
                {
                    isLongNoteAppearCompletely = true;//ロングのしっぽが出てきたかチェックする
                }

                if (isLongNoteAppearCompletely)
                {
                    if (++switchLongToNormalTimer > 15)
                    {
                        currentSpawnMode = MODE_NORMAL;
                        isLongNoteAppearCompletely = false;
                        switchLongToNormalTimer = 0;
                    }
                }
                break;
            }
        }
        //---------------------------------------------------

        //NotesUpdate
        UpdateLongNote(isLeftPressed);
        UpdateNormalNotes();

        // 入力判定
        bool currentKeys[4] = {
            (bool)(inport(PK_1) || inport(PK_Q)),
            (bool)(inport(PK_2) || inport(PK_W)),
            (bool)(inport(PK_3) || inport(PK_E)),
            (bool)(inport(PK_4) || inport(PK_R))
        };

        //NormalNotes Hit Check
        for (int lane = 0; lane < 4; lane++)
        {
            if (currentKeys[lane] == true && prevKeys[lane] == false)
            {
                bool hit = false;
                for (int i = 0; i < MAX_NOTES; i++)
                {
                    if (notes[i].active == true && notes[i].x == LANE_X[lane])
                    {
                        int diff = abs(notes[i].y - JUDGE_Y);

                        if (diff <= 1)
                        {
                            g_oldScore = g_currentScore;

                            if (diff == 0)
                            {
                                //Perfect:
                                g_currentScore += (int)((3000 + rand() % 1000) * (1 + (float)barCombo / BAR_COMBO_MAX));
                                judgeColors[lane] = GREEN;
                                g_combo++;
                                g_perfectCnt++;
                                playsound(notesHitSound, 0);
                                barCombo += 2;
                                missCount = 0;
                            }
                            else if (diff == 1)
                            {
                                // Great:
                                g_currentScore += (int)((2000 + rand() % 1000) * (1 + (float)barCombo / BAR_COMBO_MAX));
                                judgeColors[lane] = CYAN;
                                g_combo++;
                                g_greatCnt++;
                                playsound(notesHitSound, 0);
                                barCombo++;
                                missCount = 0;
                            }

                            notes[i].active = false;
                            notes[i].needsClear = true;
                            hit = true;
                            break;
                        }
                    }
                }
                if (!hit)
                {
                    judgeColors[lane] = RED;
                    g_missCnt++;
                    missCount++;
                }
                judgeTimers[lane] = 10;
            }

            prevKeys[lane] = currentKeys[lane];

            if (judgeTimers[lane] > 0)
            {
                judgeTimers[lane]--;
            }
            else
            {
                judgeColors[lane] = BLACK;
            }
        }

        //ComboMaxCnt
        if (g_combo > g_combo_MAX)
        {
            g_combo_MAX = g_combo;
        }

        //音楽
        if (isMusicStartPlaying == false)
        {
            if (gameFlag == MUSIC_NORMAL || gameFlag == MUSIC_HARD) playsound(gameMusic1, 0);
            else if (gameFlag == GUITAR_NORMAL || gameFlag == GUITAR_HARD) playsound(gameMusic2, 0);
            isMusicStartPlaying = true;
            musicStartTime = std::chrono::steady_clock::now();
        }

        if (isMusicStartPlaying == true)
        {
            auto currentTime = std::chrono::steady_clock::now();
            std::chrono::duration<float> duration = currentTime - musicStartTime;
            elapsedSec = duration.count();
            
            if (gameFlag == MUSIC_NORMAL)
            {
                if (elapsedSec < 14.0f)
                {
                    speedNormalNotesSpawn = 45;
                }
                else if (elapsedSec >= 14.0f && elapsedSec < 34.0f)
                {
                    speedNormalNotesSpawn = 35;
                }
                else if (elapsedSec >= 34.0f && elapsedSec < 58.0f)
                {
                    speedNormalNotesSpawn = 40;
                }
                else if (elapsedSec >= 58.0f && elapsedSec < 95.0f)
                {
                    speedNormalNotesSpawn = 35;
                }
                else if (elapsedSec >= 95.0f && elapsedSec < 115.0f)
                {
                    speedNormalNotesSpawn = 40;
                }
                else if (elapsedSec >= 115.0f && elapsedSec < 125.0f)
                {
                    speedNormalNotesSpawn = 45;
                }
                else
                {
                    g_spawnNormalNote = false;
                    g_spawnLongNote = false;
                    isOutro = true;
                }
            }
            else if (gameFlag == MUSIC_HARD)
            {
                if (elapsedSec < 14.0f)
                {
                    speedNormalNotesSpawn = 35;
                }
                else if (elapsedSec >= 14.0f && elapsedSec < 34.0f)
                {
                    speedNormalNotesSpawn = 25;
                }
                else if (elapsedSec >= 34.0f && elapsedSec < 58.0f)
                {
                    speedNormalNotesSpawn = 30;
                }
                else if (elapsedSec >= 58.0f && elapsedSec < 95.0f)
                {
                    speedNormalNotesSpawn = 20;
                }
                else if (elapsedSec >= 95.0f && elapsedSec < 115.0f)
                {
                    speedNormalNotesSpawn = 35;
                }
                else if (elapsedSec >= 115.0f && elapsedSec < 125.0f)
                {
                    speedNormalNotesSpawn = 40;
                }
                else
                {
                    g_spawnNormalNote = false;
                    g_spawnLongNote = false;
                    isOutro = true;
                }
            }
            else if (gameFlag == GUITAR_NORMAL)
            {
                if (elapsedSec < 10.0f)
                {
                    speedNormalNotesSpawn = 45;
                }
                else if (elapsedSec >= 10.0f && elapsedSec < 35.0f)//0:20
                {
                    speedNormalNotesSpawn = 40;
                }
                else if (elapsedSec >= 35.0f && elapsedSec < 65.0f)//1:00
                {
                    speedNormalNotesSpawn = 35;
                }
                else if (elapsedSec >= 65.0f && elapsedSec < 75.0f)//1:15
                {
                    speedNormalNotesSpawn = 40;
                }
                else if (elapsedSec >= 75.0f && elapsedSec < 85.0f)//1:25
                {
                    speedNormalNotesSpawn = 45;
                }
                else
                {
                    g_spawnNormalNote = false;
                    g_spawnLongNote = false;
                    isOutro = true;
                }
            }
            else if (gameFlag == GUITAR_HARD)
            {
                if (elapsedSec < 10.0f)
                {
                    speedNormalNotesSpawn = 30;
                }
                else if (elapsedSec >= 10.0f && elapsedSec < 35.0f)//0:20
                {
                    speedNormalNotesSpawn = 25;
                }
                else if (elapsedSec >= 35.0f && elapsedSec < 65.0f)//1:00
                {
                    speedNormalNotesSpawn = 20;
                }
                else if (elapsedSec >= 65.0f && elapsedSec < 75.0f)//1:15
                {
                    speedNormalNotesSpawn = 25;
                }
                else if (elapsedSec >= 75.0f && elapsedSec < 85.0f)//1:25
                {
                    speedNormalNotesSpawn = 30;
                }
                else
                {
                    g_spawnNormalNote = false;
                    g_spawnLongNote = false;
                    isOutro = true;
                }
            }
        }

        //MusicOver
        if (gameFlag == MUSIC_NORMAL || gameFlag == MUSIC_HARD)
        {
            if (isOvering == false && checksound(gameMusic1) == 0)
            {
                stopsound(gameMusic1);
                isOvering = true;
                overTimeCounter = 0;
            }
        }
        else if (gameFlag == GUITAR_NORMAL || gameFlag == GUITAR_HARD)
        {
            if (isOvering == false && checksound(gameMusic2) == 0)
            {
                stopsound(gameMusic2);
                isOvering = true;
                overTimeCounter = 0;
            }
        }
    }
    else if (sceneFlag == RESULT)
    {
        if (isLeftPressed && !prevLeftPressed &&
            (GetTickCount64() - resultEntryTime > 5000))
        {
            sceneFlag = HOME;
            playsound(clickSound, 0);
            playsound(homeBgm, 1);
            clrscr();

            lastFrameIcon = -1;
            isResultDrew = false;
        }
    }
    else if (sceneFlag == BONUS)
    {
        ULONGLONG currentTime = GetTickCount64();
        long long elapsed = currentTime - startTime;
        long long remaining = TIME_LIMIT_MS - elapsed;

        if (bonusBgmPlayed == false)
        {
            playsound(bonusBgm, 0);
            bonusBgmPlayed = true;
        }

        if (isCountingDown)
        {
            int nextIdx = -1;

            if (elapsed < 2000)          nextIdx = 0;
            else if (elapsed < 3000)     nextIdx = 1;
            else if (elapsed < 4000)     nextIdx = 2;
            else if (elapsed < 5000)     nextIdx = 3;
            else
            {
                isCountingDown = false;
                currentCountdownIdx = -1;
                g_oldScore = -1;
                startTime = GetTickCount64();
                ClearCanvas();
                reDrawTemplate = true;
                return;
            }

            if (nextIdx == 1 && lastPlayedIdx != 1)
            {
                playsound(countDownSound, 0);
                lastPlayedIdx = 1;
            }

            if (nextIdx != currentCountdownIdx)
            {
                currentCountdownIdx = nextIdx;
            }
            return;
        }

        //Time Check
        if (GetTickCount64() - startTime >= TIME_LIMIT_MS)
        {
            clrscr();
            sceneFlag = RESULT;
            playsound(resultBgm, 0);
            resultEntryTime = GetTickCount64();
            lastPlayedIdx = -1;
            bonusBgmPlayed = false;
            canvasEffectDrew = false;
            return;
        }

        DrawCountdownBar((int)remaining, TIME_LIMIT_MS, 77, 3, 21);

        int curX = inport(PM_CURX);
        int curY = inport(PM_CURY);
        if (isLeftPressed)
        {
            if (curX >= CANVAS_X1 && curX <= CANVAS_X2 && curY >= CANVAS_Y1 && curY <= CANVAS_Y2)
            {
                if (!g_isWriting)
                {
                    g_pathSize = 0;
                    g_isWriting = true;
                }

                bool isMovedEnough = (g_pathSize == 0);
                if (!isMovedEnough)
                {
                    int dx = abs(curX - g_gesturePath[g_pathSize - 1].x);
                    int dy = abs(curY - g_gesturePath[g_pathSize - 1].y);
                    if (dx >= 2 || dy >= 1) isMovedEnough = true;
                }

                if (isMovedEnough && g_pathSize < MAX_PATH_POINTS)
                {
                    g_gesturePath[g_pathSize].x = curX;
                    g_gesturePath[g_pathSize].y = curY;
                    g_pathSize++;
                }
            }
        }
        else
        {
            if (g_isWriting)
            {
                g_isWriting = false;
                gestureShape = AnalyzeGesture(g_gesturePath, g_pathSize);

                if (gestureShape == currentTargetShape)
                {
                    playsound(bonusCorrect, 0);
                    g_currentScore += 10000 + rand() % 5000;
                    g_seed = (g_seed * 1103515245 + 12345) & 0x7fffffff;
                    currentTargetShape = (Shape)((g_seed % 8) + 1);
                }
                else
                {
                    playsound(bonusWrong, 0);
                }

                gestureShape = NONE;
                ClearCanvas();
                reDrawTemplate = true;
                g_pathSize = 0;
            }
        }
    }

    prevLeftPressed = isLeftPressed;
}

// =============================================================================
// ゲームシーン描画
// =============================================================================
void DrawGame(void)
{
    if (sceneFlag == HOME)
    {
        if (isStartSceneDrew == false)
        {
            DrawStartScene();
            isStartSceneDrew = true;
            lastHomeColorIdx = -1;
        }

        DrawHomeEffect();

        //Start Button Breath Effect
        int speed = 2;
        int totalSteps = 24 * 2;
        int currentStep = (frameCounter / speed) % totalSteps;//0~totalSteps
        int colorIdx = (currentStep < 24) ? currentStep : (47 - currentStep);

        if (colorIdx != lastHomeColorIdx)
        {
            gotoxy(27, 22);
            std::cout << "\x1b[38;5;" << BLACK_WHITE_RAMP[colorIdx] << "m"
                << "> PRESS CLICK TO START <" << "\x1b[0m";
            lastHomeColorIdx = colorIdx;
        }
    }
    else if (sceneFlag == SELECT)
    {
        //文字
        gotoxy(27,2);
        std::cout << "\x1b[38;5;255m好きな曲と難易度を選びなさい\x1b[0m";

        //-------------------------------------------------------
        //MusicIcon
        //-------------------------------------------------------
        int currentMusicFrame = musicFocused ? (frameCounter / 30) % 2 : 0;
        bool musicNeedsRedraw = (musicFocused != lastMusicFocused) || (musicFocused && (currentMusicFrame != lastMusicFrameIdx));

        if (musicNeedsRedraw)
        {
            if (musicFocused)
            {
                if (currentMusicFrame == 0) DrawMusicFrame1(MUSIC_OFFSET_X, MUSIC_OFFSET_Y);
                else DrawMusicFrame2(MUSIC_OFFSET_X, MUSIC_OFFSET_Y);
            }
            else
            {
                DrawMusicFrame1(MUSIC_OFFSET_X, MUSIC_OFFSET_Y);
                ClearMusicFocusedEffect();
            }
            lastMusicFrameIdx = currentMusicFrame;
        }

        if (musicFocused) {
            MusicFocusedEffect();
        }
        lastMusicFocused = musicFocused;
        //-------------------------------------------------------

        //-------------------------------------------------------
        //GuitarIcon
        //-------------------------------------------------------
        int currentGuitarFrame = guitarFocused ? (frameCounter / 30) % 2 : 0;
        bool guitarNeedsRedraw = (guitarFocused != lastGuitarFocused) || (guitarFocused && (currentGuitarFrame != lastGuitarFrameIdx));

        if (guitarNeedsRedraw)
        {
            if (guitarFocused)
            {
                if (currentGuitarFrame == 0) DrawGuitarFrame1(GUITAR_OFFSET_X, GUITAR_OFFSET_Y);
                else DrawGuitarFrame2(GUITAR_OFFSET_X, GUITAR_OFFSET_Y);
            }
            else
            {
                DrawGuitarFrame1(GUITAR_OFFSET_X, GUITAR_OFFSET_Y);
                ClearGuitarFocusedEffect();
            }
            lastGuitarFrameIdx = currentGuitarFrame;
        }

        if (guitarFocused)
        {
            GuitarFocusedEffect();
        }
        lastGuitarFocused = guitarFocused;
        //-------------------------------------------------------

        //-------------------------------------------------------
        //MusicDifficultySelect
        //-------------------------------------------------------
        if (isMusicNormalFocused == true)
        {
            gotoxy(17, 19);
            std::cout << "\x1b[38;5;255m┏━━━━━━━━━┓\x1b[0m";
            gotoxy(17, 20);
            std::cout << "\x1b[38;5;255m┃ \x1b[48;5;255m\x1b[38;5;16m ふつう \x1b[0m" << "\x1b[38;5;255m┃\x1b[0m";
            gotoxy(17, 21);
            std::cout << "\x1b[38;5;255m┗━━━━━━━━━┛\x1b[0m";
        }
        else
        {
            gotoxy(17, 19);
            std::cout << "\x1b[38;5;255m┏━━━━━━━━━┓\x1b[0m";
            gotoxy(17, 20);
            std::cout << "\x1b[38;5;255m┃  ふつう ┃\x1b[0m";
            gotoxy(17, 21);
            std::cout << "\x1b[38;5;255m┗━━━━━━━━━┛\x1b[0m";
        }

        if (isMusicHardFocused == true)
        {
            gotoxy(17, 22);
            std::cout << "\x1b[38;5;255m┏━━━━━━━━━┓\x1b[0m";
            gotoxy(17, 23);
            std::cout << "\x1b[38;5;255m┃ \x1b[48;5;255m\x1b[38;5;16mたつじん\x1b[0m" << "\x1b[38;5;255m┃\x1b[0m";
            gotoxy(17, 24);
            std::cout << "\x1b[38;5;255m┗━━━━━━━━━┛\x1b[0m";
        }
        else
        {
            gotoxy(17, 22);
            std::cout << "\x1b[38;5;255m┏━━━━━━━━━┓\x1b[0m";
            gotoxy(17, 23);
            std::cout << "\x1b[38;5;255m┃ たつじん┃\x1b[0m";
            gotoxy(17, 24);
            std::cout << "\x1b[38;5;255m┗━━━━━━━━━┛\x1b[0m";
        }
        //-------------------------------------------------------

        //-------------------------------------------------------
        //GuitarDifficultySelect
        //-------------------------------------------------------
        if (isGuitarNormalFocused == true)
        {
            gotoxy(53, 19);
            std::cout << "\x1b[38;5;255m┏━━━━━━━━━┓\x1b[0m";
            gotoxy(53, 20);
            std::cout << "\x1b[38;5;255m┃ \x1b[48;5;255m\x1b[38;5;16m ふつう \x1b[0m" << "\x1b[38;5;255m┃\x1b[0m";
            gotoxy(53, 21);
            std::cout << "\x1b[38;5;255m┗━━━━━━━━━┛\x1b[0m";
        }
        else
        {
            gotoxy(53, 19);
            std::cout << "\x1b[38;5;255m┏━━━━━━━━━┓\x1b[0m";
            gotoxy(53, 20);
            std::cout << "\x1b[38;5;255m┃  ふつう ┃\x1b[0m";
            gotoxy(53, 21);
            std::cout << "\x1b[38;5;255m┗━━━━━━━━━┛\x1b[0m";
        }

        if (isGuitarHardFocused == true)
        {
            gotoxy(53, 22);
            std::cout << "\x1b[38;5;255m┏━━━━━━━━━┓\x1b[0m";
            gotoxy(53, 23);
            std::cout << "\x1b[38;5;255m┃ \x1b[48;5;255m\x1b[38;5;16mたつじん\x1b[0m" << "\x1b[38;5;255m┃\x1b[0m";
            gotoxy(53, 24);
            std::cout << "\x1b[38;5;255m┗━━━━━━━━━┛\x1b[0m";
        }
        else
        {
            gotoxy(53, 22);
            std::cout << "\x1b[38;5;255m┏━━━━━━━━━┓\x1b[0m";
            gotoxy(53, 23);
            std::cout << "\x1b[38;5;255m┃ たつじん┃\x1b[0m";
            gotoxy(53, 24);
            std::cout << "\x1b[38;5;255m┗━━━━━━━━━┛\x1b[0m";
        }
        //-------------------------------------------------------
    }
    else if (sceneFlag == GAME)
    {
        //-------- Draw Static UI Only Once ---------------
        if (isStaticUIDrew == false)
        {
            //Static UI
            textbackground(BLACK);
            textcolor(WHITE);
            for (int y = 1; y < LIMIT_Y; y++)
            {
                textcolor(DARKGRAY);
                gotoxy(1, y);  std::cout << "|";
                gotoxy(LANE_X[0] - 1, y);  std::cout << "|";
                gotoxy(LANE_X[3] + 10, y); std::cout << "|";
                gotoxy(LANE_X[3] + 12, y); std::cout << "|";
            }
            //Judge Line
            for (int lane = 0; lane < 4; lane++)
            {
                //textcolor(DARKGRAY);
                //textbackground(BLACK);
                //gotoxy(LANE_X[lane], JUDGE_Y - 1); std::cout << "- - -- - -";
                //gotoxy(LANE_X[lane], JUDGE_Y + 1); std::cout << "- - -- - -";

                textcolor(WHITE);
                textbackground(BLACK);
                gotoxy(LANE_X[lane], JUDGE_Y);     std::cout << "==========";
            }

            //line
            gotoxy(50, 8);
            std::cout << " - - - - - - - - - - - - - - -";
            //POWER gauge
            gotoxy(50, 9);
            std::cout << "┏━━━━━━━━━━  パワー ━━━━━━━━━┓";
            gotoxy(50, 10);
            std::cout << "┃";//27個スペース
            gotoxy(79, 10);
            std::cout << "┃";
            gotoxy(50, 11);
            std::cout << "┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━┛";

            //Emoji
            gotoxy(50, 12);
            std::cout << "┏━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓";
            for (int i = 0; i < 12; i++)
            {
                gotoxy(50, 13 + i);
                std::cout << "┃";
                gotoxy(79, 13 + i);
                std::cout << "┃";
            }
            gotoxy(50, 25);
            std::cout << "┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━┛";

            isStaticUIDrew = true;
        }

        //---------------------------------------------------
        //Power Gauge
        DrawPowerGauge();

        //---------------------------------------------------
        //Combo Emoji
        DrawEmojis();
        //---------------------------------------------------

        //---------------------------------------------------
        //ScoreBoard
        if (g_currentScore != g_oldScore)
        {
            DrawScoreBoard(DIGIT_X, DIGIT_Y, g_currentScore);
            g_oldScore = g_currentScore;
        }
        DrawScoreNeonEffect();

        //---------------------------------------------------
        if (barCombo <= 0) barCombo = 0;
        if (barCombo >= BAR_COMBO_MAX) barCombo = BAR_COMBO_MAX;
        DrawHeatBar();

        //---------------------------------------------------
        //終了段階
        if (isOvering == true)
        {
            if (isOverDrew == false)
            {
                for (int y = 1; y <= 25; y++)
                {
                    gotoxy(4, y);
                    std::cout << "                                           ";
                }
                DrawOverHint();
                playsound(overDrum, 0);
                isOverDrew = true;
            }
            overTimeCounter++;
            if (overTimeCounter % 240 == 0) readyToResult = true;
            return;
        }
        //---------------------------------------------------

        //---------------------------------------------------
        //判定ラインの描画
        for (int lane = 0; lane < 4; lane++)
        {
            if (judgeTimers[lane] > 0 || frameCounter % beatNum == 0)
            {
                textcolor(WHITE);
                textbackground(judgeColors[lane]);
                gotoxy(LANE_X[lane], JUDGE_Y);
                std::cout << "==========";
                textbackground(BLACK);
            }
        }

        DrawLongNote();
        DrawNormalNotes();
    }
    else if (sceneFlag == RESULT)
    {
        //Title
        if (isResultDrew == false)
        {
            DrawResult();
            isResultDrew = true;
        }

        //Icon
        int currentFrame = (frameCounter / 30) % 2;
        bool needsRedraw = (currentFrame != lastFrameIcon);

        if (needsRedraw == true)
        {
            switch (iconSelected)
            {
            case MUSIC:
                if (currentFrame == 0) DrawMusicFrame1(13, 10);
                else DrawMusicFrame2(13, 10);
                break;
            case GUITAR:
                if (currentFrame == 0) DrawGuitarFrame1(13, 10);
                else DrawGuitarFrame2(13, 10);
                break;
            }
            lastFrameIcon = currentFrame;
        }

        //Result
        gotoxy(45, 10);
        std::cout << "\x1b[38;5;255m┏━━━━━━━━━━━━━━━━━━┓\x1b[0m";

        //--- Perfect ---
        gotoxy(45, 12);
        std::cout << "\x1b[38;5;255m┃  Perfect: " << g_perfectCnt;
        gotoxy(64, 12);
        std::cout << "┃\x1b[0m";

        //--- Great ---
        gotoxy(45, 14);
        std::cout << "\x1b[38;5;255m┃  Great  : " << g_greatCnt;
        gotoxy(64, 14);
        std::cout << "┃\x1b[0m";

        //--- Miss ---
        gotoxy(45, 16);
        std::cout << "\x1b[38;5;255m┃  Miss   : " << g_missCnt;
        gotoxy(64, 16);
        std::cout << "┃\x1b[0m";

        //--- Score ---
        gotoxy(45, 18);
        std::cout << "\x1b[38;5;255m┃  スコア : " << g_currentScore;
        gotoxy(64, 18);
        std::cout << "┃\x1b[0m";

        //--- Combo ---
        gotoxy(45, 20);
        std::cout << "\x1b[38;5;255m┃  連打数 : " << g_combo_MAX;
        gotoxy(64, 20);
        std::cout << "┃\x1b[0m";

        gotoxy(45, 22);
        std::cout << "\x1b[38;5;255m┗━━━━━━━━━━━━━━━━━━┛\x1b[0m";
    }
    else if (sceneFlag == BONUS)
    {
        if (isCountingDown)
        {
            if (currentCountdownIdx >= 0)
            {
                DrawCountDowns(currentCountdownIdx);
            }
        }
        else
        {
            if (canvasEffectDrew == false)
            {
                for (int y = 8; y <= 19; y++)
                {
                    gotoxy(1, y);
                    DrawBonusEffect(29, true);
                    
                    gotoxy(50, y);
                    DrawBonusEffect(31, false);
                }
                canvasEffectDrew = true;
            }

            if (g_currentScore != g_oldScore)
            {
                DrawScoreBoard(27, 2, g_currentScore);
                g_oldScore = g_currentScore;
            }

            if (reDrawTemplate)
            {
                gotoxy(1, 20);
                std::cout << "┏━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓";
                gotoxy(1, 21);
                std::cout << "┃";
                gotoxy(80, 21);
                std::cout << "┃";
                gotoxy(1, 22);
                std::cout << "┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┛";
                
                DrawTemplate(currentTargetShape);
                reDrawTemplate = false;
            }

            for (int i = 0; i < g_pathSize; i++)
            {
                int x = g_gesturePath[i].x;
                int y = g_gesturePath[i].y;
                if (x >= CANVAS_X1 && x <= CANVAS_X2 && y >= CANVAS_Y1 && y <= CANVAS_Y2)
                {
                    int tx = x - 31;
                    int ty = y - 9;

                    gotoxy(x, y);
                    if (tx >= 0 && tx < 19 && ty >= 0 && ty < 10 &&
                        templateData[currentTargetShape][ty][tx] == 1)
                    {
                        std::cout << "\x1b[30;48;5;8m*\x1b[0m";
                    }
                    else
                    {
                        gotoxy(x, y);
                        std::cout << "\x1b[30;48;5;255m*\x1b[0m";
                    }
                }
            }
        }
    }
}

// =============================================================================
// ゲームシーン終了処理
// =============================================================================
void FinalizeGame(void)
{
    closesound(gameMusic1);
    closesound(gameMusic2);
    closesound(homeBgm);
    closesound(countDownSound);
    closesound(bonusBgm);
    closesound(bonusCorrect);
    closesound(bonusWrong);
    closesound(resultBgm);
    closesound(clickSound);
    closesound(overDrum);
    closesound(notesHitSound);
}

//Personal Methods
//Start Scene
void LoadStartScene()
{
    std::ifstream file("START_SCENE.csv");
    if (!file) return;

    char ch;
    int row = 0, col = 0;

    while (file.get(ch) && row < 25)
    {
        if (ch == '0' || ch == '1' || ch == '2' || ch == '3')
        {
            startSceneData[row][col] = ch - '0';
            col++;
            if (col >= 80)
            {
                col = 0;
                row++;
            }
        }
    }
}
void DrawStartScene()
{
    for (int y = 0; y < 25; y++)
    {
        for (int x = 0; x < 80; x++)
        {
            if (startSceneData[y][x] == 1)
            {
                gotoxy(x, y);
                std::cout << "\x1b[48;5;255m ";
            }
            else if (startSceneData[y][x] == 2)
            {
                gotoxy(x, y);
                std::cout << "\x1b[48;5;214m ";
            }
            else if (startSceneData[y][x] == 3)
            {
                gotoxy(x, y);
                std::cout << "\x1b[48;5;10m ";
            }
        }
    }

    std::cout << "\x1b[0m";
}
void DrawHomeEffect()
{
    if (frameCounter % 2 != 0) return;

    int pathSize = GetPathSize(SCR_X1, SCR_Y1, SCR_X2, SCR_Y2);

    int headSlower = 2;
    int headPos1 = (frameCounter / headSlower) % pathSize;
    int headPos2 = (headPos1 + pathSize / 2) % pathSize;

    //Get old tail posX and posY,then cout a black space
    int tx, ty;
    GetNeonXY(headPos1 - HOME_EFFECT_TAIL, SCR_X1, SCR_Y1, SCR_X2, SCR_Y2, tx, ty);
    gotoxy(tx, ty); std::cout << "\x1b[0m ";

    GetNeonXY(headPos2 - HOME_EFFECT_TAIL, SCR_X1, SCR_Y1, SCR_X2, SCR_Y2, tx, ty);
    gotoxy(tx, ty); std::cout << "\x1b[0m ";

    //Draw the neon effect
    for (int i = 0; i < HOME_EFFECT_TAIL; i++)
    {
        int posX, posY;

        int colorIdx = i * (6 - 1) / (HOME_EFFECT_TAIL - 1);
        if (colorIdx < 0) colorIdx = 0;
        if (colorIdx >= 6) colorIdx = 5;

        //Meteor1
        GetNeonXY(headPos1 - (HOME_EFFECT_TAIL - 1 - i), SCR_X1, SCR_Y1, SCR_X2, SCR_Y2, posX, posY);
        gotoxy(posX, posY);
        std::cout << "\x1b[48;5;" << HEAT_RAMP[colorIdx] << "m ";

        //Meteor2
        GetNeonXY(headPos2 - (HOME_EFFECT_TAIL - 1 - i), SCR_X1, SCR_Y1, SCR_X2, SCR_Y2, posX, posY);
        gotoxy(posX, posY);
        std::cout << "\x1b[48;5;" << BLUE_RAMP[colorIdx] << "m ";
    }

    //Setting back to default
    std::cout << "\x1b[0m";
}

//Select Scene
void LoadMusicFrame1()
{
    std::ifstream file("MUSIC_FRAME_1.csv");
    if (!file) return;

    char ch;
    int row = 0, col = 0;

    while (file.get(ch) && row < 13)
    {
        if (ch == '1' || ch == '2' || ch == '3' || ch == '4' || ch == '5')
        {
            musicIconFrame1[row][col] = ch - '0';
            col++;
            if (col >= 24)
            {
                col = 0;
                row++;
            }
        }
    }
}
void DrawMusicFrame1(int offset_x, int offset_y)
{
    for (int y = 0; y < 13; y++)
    {
        for (int x = 0; x < 24; x++)
        {
            if (musicIconFrame1[y][x] == 1)//White
            {
                gotoxy(x + offset_x, y + offset_y);
                std::cout << "\x1b[48;5;255m ";
            }
            else if (musicIconFrame1[y][x] == 2)//Black
            {
                gotoxy(x + offset_x, y + offset_y);
                std::cout << "\x1b[48;5;232m ";
            }
            else if (musicIconFrame1[y][x] == 3)//Red
            {
                gotoxy(x + offset_x, y + offset_y);
                std::cout << "\x1b[48;5;1m ";
            }
            else if (musicIconFrame1[y][x] == 4)//LightRed
            {
                gotoxy(x + offset_x, y + offset_y);
                std::cout << "\x1b[48;5;9m ";
            }
        }
    }

    std::cout << "\x1b[0m";
}
void LoadMusicFrame2()
{
    std::ifstream file("MUSIC_FRAME_2.csv");
    if (!file) return;

    char ch;
    int row = 0, col = 0;

    while (file.get(ch) && row < 13)
    {
        if (ch == '1' || ch == '2' || ch == '3' || ch == '4' || ch == '5')
        {
            musicIconFrame2[row][col] = ch - '0';
            col++;
            if (col >= 24)
            {
                col = 0;
                row++;
            }
        }
    }
}
void DrawMusicFrame2(int offset_x, int offset_y)
{
    for (int y = 0; y < 13; y++)
    {
        for (int x = 0; x < 24; x++)
        {
            if (musicIconFrame2[y][x] == 1)//White
            {
                gotoxy(x + offset_x, y + offset_y);
                std::cout << "\x1b[48;5;255m ";
            }
            else if (musicIconFrame2[y][x] == 2)//Black
            {
                gotoxy(x + offset_x, y + offset_y);
                std::cout << "\x1b[48;5;232m ";
            }
            else if (musicIconFrame2[y][x] == 3)//Red
            {
                gotoxy(x + offset_x, y + offset_y);
                std::cout << "\x1b[48;5;1m ";
            }
            else if (musicIconFrame2[y][x] == 4)//LightRed
            {
                gotoxy(x + offset_x, y + offset_y);
                std::cout << "\x1b[48;5;9m ";
            }
            else if (musicIconFrame2[y][x] == 5)//LightPink
            {
                gotoxy(x + offset_x, y + offset_y);
                std::cout << "\x1b[48;5;204m ";
            }
        }
    }

    std::cout << "\x1b[0m";
}

void MusicFocusedEffect()
{
    if (frameCounter % 2 != 0) return;

    int pathSize = GetPathSize(MUSIC_X1, MUSIC_Y1, MUSIC_X2, MUSIC_Y2);

    int headSlower = 2;
    int headPos1 = (frameCounter / headSlower) % pathSize;
    int headPos2 = (headPos1 + pathSize / 2) % pathSize;

    //Get old tail posX and posY,then cout a black space
    int tx, ty;
    GetNeonXY(headPos1 - ICON_EFFECT_TAIL, MUSIC_X1, MUSIC_Y1, MUSIC_X2, MUSIC_Y2, tx, ty);
    gotoxy(tx, ty); std::cout << "\x1b[0m ";

    GetNeonXY(headPos2 - ICON_EFFECT_TAIL, MUSIC_X1, MUSIC_Y1, MUSIC_X2, MUSIC_Y2, tx, ty);
    gotoxy(tx, ty); std::cout << "\x1b[0m ";

    //Draw the neon effect
    for (int i = 0; i < ICON_EFFECT_TAIL; i++)
    {
        int posX, posY;

        int colorIdx = i * (6 - 1) / (ICON_EFFECT_TAIL - 1);
        if (colorIdx < 0) colorIdx = 0;
        if (colorIdx >= 6) colorIdx = 5;

        //Meteor1
        GetNeonXY(headPos1 - i, MUSIC_X1, MUSIC_Y1, MUSIC_X2, MUSIC_Y2, posX, posY);
        gotoxy(posX, posY);
        std::cout << "\x1b[48;5;" << HEAT_RAMP[colorIdx] << "m ";

        //Meteor2
        GetNeonXY(headPos2 - i, MUSIC_X1, MUSIC_Y1, MUSIC_X2, MUSIC_Y2, posX, posY);
        gotoxy(posX, posY);
        std::cout << "\x1b[48;5;" << HEAT_RAMP[colorIdx] << "m ";
    }

    //Setting back to default
    std::cout << "\x1b[0m";
}
void ClearMusicFocusedEffect()
{
    int pathSize = GetPathSize(MUSIC_X1, MUSIC_Y1, MUSIC_X2, MUSIC_Y2);

    std::cout << "\x1b[0m";
    for (int i = 0; i < pathSize; i++)
    {
        int tx, ty;
        GetNeonXY(i, MUSIC_X1, MUSIC_Y1, MUSIC_X2, MUSIC_Y2, tx, ty);
        gotoxy(tx, ty);
        std::cout << " ";
    }
}

void LoadGuitarFrame1()
{
    std::ifstream file("GUITAR_FRAME_1.csv");
    if (!file) return;

    char ch;
    int row = 0, col = 0;

    while (file.get(ch) && row < 13)
    {
        if (ch == '1' || ch == '2' || ch == '3' || ch == '4' || ch == '5' || ch == '6' || ch == '7' || ch == '8')
        {
            guitarIconFrame1[row][col] = ch - '0';
            col++;
            if (col >= 24)
            {
                col = 0;
                row++;
            }
        }
    }
}
void DrawGuitarFrame1(int offset_x, int offset_y)
{
    for (int y = 0; y < 13; y++)
    {
        for (int x = 0; x < 24; x++)
        {
            if (guitarIconFrame1[y][x] == 1)//White
            {
                gotoxy(x + offset_x, y + offset_y);
                std::cout << "\x1b[48;5;255m ";
            }
            else if (guitarIconFrame1[y][x] == 2)//Black
            {
                gotoxy(x + offset_x, y + offset_y);
                std::cout << "\x1b[48;5;232m ";
            }
            else if (guitarIconFrame1[y][x] == 3)//Grey
            {
                gotoxy(x + offset_x, y + offset_y);
                std::cout << "\x1b[48;5;7m ";
            }
            else if (guitarIconFrame1[y][x] == 4)//DarkBlue
            {
                gotoxy(x + offset_x, y + offset_y);
                std::cout << "\x1b[48;5;19m ";
            }
            else if (guitarIconFrame1[y][x] == 5)//Blue
            {
                gotoxy(x + offset_x, y + offset_y);
                std::cout << "\x1b[48;5;21m ";
            }
            else if (guitarIconFrame1[y][x] == 6)//Brown
            {
                gotoxy(x + offset_x, y + offset_y);
                std::cout << "\x1b[48;5;130m ";
            }
            else if (guitarIconFrame1[y][x] == 7)//DarkBrown
            {
                gotoxy(x + offset_x, y + offset_y);
                std::cout << "\x1b[48;5;94m ";
            }
            else if (guitarIconFrame1[y][x] == 8)//LightBrown
            {
                gotoxy(x + offset_x, y + offset_y);
                std::cout << "\x1b[48;5;3m ";
            }
        }
    }

    std::cout << "\x1b[0m";
}
void LoadGuitarFrame2()
{
    std::ifstream file("GUITAR_FRAME_2.csv");
    if (!file) return;

    char ch;
    int row = 0, col = 0;

    while (file.get(ch) && row < 13)
    {
        if (ch == '1' || ch == '2' || ch == '3' || ch == '4' || ch == '5' || ch == '6' || ch == '7' || ch == '8')
        {
            guitarIconFrame2[row][col] = ch - '0';
            col++;
            if (col >= 24)
            {
                col = 0;
                row++;
            }
        }
    }
}
void DrawGuitarFrame2(int offset_x, int offset_y)
{
    for (int y = 0; y < 13; y++)
    {
        for (int x = 0; x < 24; x++)
        {
            if (guitarIconFrame2[y][x] == 1)//White
            {
                gotoxy(x + offset_x, y + offset_y);
                std::cout << "\x1b[48;5;255m ";
            }
            else if (guitarIconFrame2[y][x] == 2)//Black
            {
                gotoxy(x + offset_x, y + offset_y);
                std::cout << "\x1b[48;5;232m ";
            }
            else if (guitarIconFrame2[y][x] == 3)//Grey
            {
                gotoxy(x + offset_x, y + offset_y);
                std::cout << "\x1b[48;5;7m ";
            }
            else if (guitarIconFrame2[y][x] == 4)//DarkBlue
            {
                gotoxy(x + offset_x, y + offset_y);
                std::cout << "\x1b[48;5;19m ";
            }
            else if (guitarIconFrame2[y][x] == 5)//Blue
            {
                gotoxy(x + offset_x, y + offset_y);
                std::cout << "\x1b[48;5;21m ";
            }
            else if (guitarIconFrame2[y][x] == 6)//Brown
            {
                gotoxy(x + offset_x, y + offset_y);
                std::cout << "\x1b[48;5;130m ";
            }
            else if (guitarIconFrame2[y][x] == 7)//DarkBrown
            {
                gotoxy(x + offset_x, y + offset_y);
                std::cout << "\x1b[48;5;94m ";
            }
            else if (guitarIconFrame2[y][x] == 8)//LightBrown
            {
                gotoxy(x + offset_x, y + offset_y);
                std::cout << "\x1b[48;5;3m ";
            }
        }
    }

    std::cout << "\x1b[0m";
}

void GuitarFocusedEffect()
{
    if (frameCounter % 2 != 0) return;

    int pathSize = GetPathSize(GUITAR_X1, GUITAR_Y1, GUITAR_X2, GUITAR_Y2);

    int headSlower = 2;
    int headPos1 = (frameCounter / headSlower) % pathSize;
    int headPos2 = (headPos1 + pathSize / 2) % pathSize;

    //Get old tail posX and posY,then cout a black space
    int tx, ty;
    GetNeonXY(headPos1 - ICON_EFFECT_TAIL, GUITAR_X1, GUITAR_Y1, GUITAR_X2, GUITAR_Y2, tx, ty);
    gotoxy(tx, ty); std::cout << "\x1b[0m ";

    GetNeonXY(headPos2 - ICON_EFFECT_TAIL, GUITAR_X1, GUITAR_Y1, GUITAR_X2, GUITAR_Y2, tx, ty);
    gotoxy(tx, ty); std::cout << "\x1b[0m ";

    //Draw the neon effect
    for (int i = 0; i < ICON_EFFECT_TAIL; i++)
    {
        int posX, posY;

        int colorIdx = i * (6 - 1) / (ICON_EFFECT_TAIL - 1);
        if (colorIdx < 0) colorIdx = 0;
        if (colorIdx >= 6) colorIdx = 5;

        //Meteor1
        GetNeonXY(headPos1 - i, GUITAR_X1, GUITAR_Y1, GUITAR_X2, GUITAR_Y2, posX, posY);
        gotoxy(posX, posY);
        std::cout << "\x1b[48;5;" << BLUE_RAMP[colorIdx] << "m ";

        //Meteor2
        GetNeonXY(headPos2 - i, GUITAR_X1, GUITAR_Y1, GUITAR_X2, GUITAR_Y2, posX, posY);
        gotoxy(posX, posY);
        std::cout << "\x1b[48;5;" << BLUE_RAMP[colorIdx] << "m ";
    }

    //Setting back to default
    std::cout << "\x1b[0m";

}
void ClearGuitarFocusedEffect()
{
    int pathSize = GetPathSize(GUITAR_X1, GUITAR_Y1, GUITAR_X2, GUITAR_Y2);

    std::cout << "\x1b[0m";
    for (int i = 0; i < pathSize; i++)
    {
        int tx, ty;
        GetNeonXY(i, GUITAR_X1, GUITAR_Y1, GUITAR_X2, GUITAR_Y2, tx, ty);
        gotoxy(tx, ty);
        std::cout << " ";
    }
}

//Fill Note Color
void FillColorBar(int x, int y, int width, int color256)
{
    if (y < 1 || y >= LIMIT_Y) return;
    gotoxy(x, y);
    std::cout << "\x1b[48;5;" << color256 << "m" << std::string(width, ' ') << "\x1b[0m";
}

//-----Normal Notes-----
void UpdateNormalNotes()
{
    //20フレームずつ判断して生成する
    if (frameCounter % speedNormalNotesSpawn == 0 && g_spawnNormalNote == true)
    {
        //Notes spawn
        for (int i = 0; i < MAX_NOTES; i++)
        {
            if (notes[i].active == false && notes[i].needsClear == false)
            {
                notes[i].active = true;
                notes[i].x = LANE_X[rand() % 4];
                notes[i].y = 0;
                notes[i].oldY = 0;
                break;
            }
        }
    }

    //Notes Moves
    for (int i = 0; i < MAX_NOTES; i++)
    {
        if (!notes[i].active && notes[i].needsClear == false) continue;

        notes[i].oldY = notes[i].y;

        //落下速度
        if (frameCounter % beatNum == 0)
        {
            notes[i].y++;
        }

        if (notes[i].y >= LIMIT_Y)
        {
            notes[i].active = false;
            notes[i].needsClear = true;
            barCombo -= 10;
            g_combo = 0;
            missCount++;

            g_missCnt++;
        }
    }
}
void DrawNormalNotes()
{
    for (int i = 0; i < MAX_NOTES; i++)
    {
        if (!notes[i].active && !notes[i].needsClear) continue;

        // Notes Delete
        if (notes[i].needsClear || (notes[i].active && notes[i].y != notes[i].oldY))
        {
            FillColorBar(notes[i].x, notes[i].oldY, 10, 0);

            // ------- Line Repair ---------
            //if (notes[i].oldY == JUDGE_Y - 1 || notes[i].oldY == JUDGE_Y + 1)
            //{
            //    textcolor(DARKGRAY);
            //    gotoxy(notes[i].x, notes[i].oldY);
            //    std::cout << "- - -- - -";
            //}
            // -----------------------------
            
            notes[i].needsClear = false;
        }

        // Notes Draw
        if (notes[i].active)
        {
            FillColorBar(notes[i].x, notes[i].y, 10, 15);
        }
    }
}

//-----Long Note-----
void UpdateLongNote(bool isPress)
{
    if (g_spawnLongNote && !LNote.active)
    {
        LNote.active = true;
        LNote.startLaneX = LANE_X[rand() % 4];
        LNote.endLaneX = LANE_X[rand() % 4];
        LNote.amplitude = 11;

        for (int i = 0; i < LNOTE_LEN; i++)
        {
            float t = (float)i / (LNOTE_LEN - 1);
            int baseCells = (int)(LNote.startLaneX + t * (LNote.endLaneX - LNote.startLaneX));
            int offsetX = 0;

            if (LNote.startLaneX == LNote.endLaneX)
            {
                if (LNote.startLaneX == LANE_X[0] || LNote.startLaneX == LANE_X[3])
                {
                    LNote.pathType = L_LINEAR;
                }
                else
                {
                    LNote.pathType = L_ZIGZAG;
                }
            }
            else
            {
                LNote.pathType = L_CURVE;
            }

            //PathType
            switch (LNote.pathType)
            {
            case L_LINEAR:
                offsetX = 0;
                break;
            case L_ZIGZAG:
                offsetX = (t < 0.5f) ? (int)(LNote.amplitude * t * 2) : (int)(LNote.amplitude * (1.0f - t) * 2);
                break;
            case L_CURVE:
                float easeT = t * t * (3.0f - 2.0f * t);
                baseCells = (int)(LNote.startLaneX + easeT * (LNote.endLaneX - LNote.startLaneX));
                offsetX = 0;
                break;
            }

            LNote.x[i] = baseCells + offsetX;
            LNote.y[i] = -i;
            LNote.oldX[i] = LNote.x[i];
            LNote.oldY[i] = LNote.y[i];
            LNote.isHit[i] = false;
            {
                float progress = (float)i / (LNOTE_LEN - 1); // 0.0 -> 1.0
                float rate = fabsf(progress - 0.5f) * 2.0f;  // 1 -> 0 -> 1
                LNote.bgColor256[i] = 240 + (int)(rate * (255 - 240));
            }

        }
    }
    if (!LNote.active) return;

    //LongNote Movement
    if (frameCounter % beatNum == 0)
    {
        //Save old position
        for (int i = 0; i < LNOTE_LEN; i++)
        {
            LNote.oldX[i] = LNote.x[i];
            LNote.oldY[i] = LNote.y[i];
            LNote.y[i]++;
        }
    }

    //Mouse hold left key for judging notes
    int mouseX = inport(PM_CURX);
    int mouseY = inport(PM_CURY);

    LNote.isHolding = false;

    if (isPress && abs(mouseY - JUDGE_Y) <= 1)
    {
        for (int i = 0; i < LNOTE_LEN; i++)
        {
            if (LNote.y[i] == JUDGE_Y && !LNote.isHit[i])
            {
                if (mouseX >= LNote.x[i] - 1 && mouseX < LNote.x[i] + 11)
                {
                    g_oldScore = g_currentScore;
                    g_currentScore += 1000 + rand() % 500;

                    barCombo++;
                    g_combo++;
                    g_perfectCnt++;
                    missCount = 0;

                    LNote.isHit[i] = true;
                    LNote.isHolding = true;
                    break;
                }
                else
                {
                    missCount++;
                    barCombo -= 5;
                    g_combo = 0;

                    g_missCnt++;
                    break;
                }
            }
        }
    }

    //Delete when over the judge line
    if (LNote.y[LNOTE_LEN - 1] >= LIMIT_Y)
    {
        //If perfect All Hit
        bool allHit = true;
        for (int i = 0; i < LNOTE_LEN; i++)
        {
            if (!LNote.isHit[i])
            {
                allHit = false;
                break;
            }
        }
        //Perfecr All Hit Extra Bonus
        if (allHit && LNote.active)
        {
            playsound(notesHitSound, 0);
            g_oldScore = g_currentScore;
            g_currentScore += 2000 + rand() % 3000;
            barCombo += 5;
            g_perfectCnt++;
            g_combo++;
        }

        LNote.active = false;
        LNote.needsClear = true;
    }
}
void DrawLongNote()
{
    if (!LNote.active && !LNote.needsClear) return;
    bool isMovingFrame = (frameCounter % beatNum == 0);

    bool needRepairTop = false;
    bool needRepairBot = false;

    //If not active and needsClear
    if ((LNote.needsClear && !LNote.active) || isMovingFrame)
    {
        for (int i = 0; i < LNOTE_LEN; i++)
        {
            if (LNote.oldY[i] >= 0)
            {
                FillColorBar(LNote.oldX[i], LNote.oldY[i], 10, 0);

                // Line Repair
                //if (LNote.oldY[i] == JUDGE_Y - 1) needRepairTop = true;
                //if (LNote.oldY[i] == JUDGE_Y + 1) needRepairBot = true;
            }
        }

        //if (needRepairTop || needRepairBot)
        //{
        //    textcolor(DARKGRAY);
        //    textbackground(BLACK);
        //    for (int cnt = 0; cnt < 4; cnt++)
        //    {
        //        if (needRepairTop) {
        //            gotoxy(LANE_X[cnt], JUDGE_Y - 1);
        //            std::cout << "- - -- - -";
        //        }
        //        if (needRepairBot) {
        //            gotoxy(LANE_X[cnt], JUDGE_Y + 1);
        //            std::cout << "- - -- - -";
        //        }
        //    }
        //}

        if (LNote.needsClear && !LNote.active)
        {
            LNote.needsClear = false;
            return;
        }
    }

    //if is going to move or is holding, should do the delete and draw again
    if (isMovingFrame || LNote.isHolding)
    {
        for (int i = 0; i < LNOTE_LEN; i++)
        {
            if (LNote.y[i] < LIMIT_Y && LNote.oldY[i] >= 0)
            {
                if (LNote.isHit[i] && LNote.y[i] > JUDGE_Y) continue;
                if (!isMovingFrame && LNote.y[i] != JUDGE_Y) continue;

                int bgColor = LNote.bgColor256[i];
                if (LNote.y[i] == JUDGE_Y && LNote.isHit[i])
                {
                    bgColor = 2; // Green
                }

                gotoxy(LNote.x[i], LNote.y[i]);

                if (i == 0 || i == LNOTE_LEN - 1)
                {
                    printf("\x1b[48;5;%dm\x1b[38;5;0m<   ○   >\x1b[0m", bgColor);
                }
                else
                {
                    printf("\x1b[48;5;%dm\x1b[38;5;0m          \x1b[0m", bgColor);
                }
            }
        }
    }
}

//-----Score Board-----
void DrawLargeDigit(int startX, int startY, int digit)
{
    for (int row = 0; row < 5; row++)
    {
        gotoxy(startX, startY + row);
        for (int col = 0; col < 3; col++)
        {
            if (DIGIT_PATTERNS[digit][row][col] == 1)
            {
                std::cout << "\x1b[48;5;255m \x1b[0m";
            }
            else
            {
                std::cout << "\x1b[48;5;16m \x1b[0m";
            }
        }
    }
}
void DrawScoreBoard(int x, int y, long score)
{
    if (score > 9999999) score = 9999999;//MAX SCORE

    int displayDigits[7];
    long temp = score;

    for (int i = 6; i >= 0; i--)
    {
        displayDigits[i] = temp % 10;
        temp /= 10;
    }

    for (int i = 0; i < 7; i++)
    {
        DrawLargeDigit(x + (i * 4), y, displayDigits[i]);
    }

    textbackground(BLACK);
}

//Score Board Effect
void DrawScoreNeonEffect()
{
    if (frameCounter % 2 != 0) return;

    int pathSize = GetPathSize(BORDER_X1, BORDER_Y1, BORDER_X2, BORDER_Y2);

    int headSlower = 2;
    int headPos1 = (frameCounter / headSlower) % pathSize;
    int headPos2 = (headPos1 + pathSize / 2) % pathSize;

    //Get old tail posX and posY,then cout a black space
    int tailX, tailY;
    GetNeonXY(headPos1 - NEON_TAIL, BORDER_X1, BORDER_Y1, BORDER_X2, BORDER_Y2, tailX, tailY);
    gotoxy(tailX, tailY); std::cout << "\x1b[0m ";

    GetNeonXY(headPos2 - NEON_TAIL, BORDER_X1, BORDER_Y1, BORDER_X2, BORDER_Y2, tailX, tailY);
    gotoxy(tailX, tailY); std::cout << "\x1b[0m ";

    //Draw the neon effect
    for (int i = 0; i < NEON_TAIL; i++)
    {
        int posX, posY;

        int colorIdx = i * (24 - 1) / (NEON_TAIL - 1);
        if (colorIdx < 0) colorIdx = 0;
        if (colorIdx >= 24) colorIdx = 23;

        //Meteor1
        GetNeonXY(headPos1 - (NEON_TAIL - 1 - i), BORDER_X1, BORDER_Y1, BORDER_X2, BORDER_Y2, posX, posY);
        gotoxy(posX, posY);
        std::cout << "\x1b[48;5;" << BLACK_WHITE_RAMP[colorIdx] << "m ";

        //Meteor2
        GetNeonXY(headPos2 - (NEON_TAIL - 1 - i), BORDER_X1, BORDER_Y1, BORDER_X2, BORDER_Y2, posX, posY);
        gotoxy(posX, posY);
        std::cout << "\x1b[48;5;" << BLACK_WHITE_RAMP[colorIdx] << "m ";
    }

    //Setting back to default
    std::cout << "\x1b[0m";
}
int GetPathSize(int x1, int y1, int x2, int y2)
{
    return ((x2 - x1) + (y2 - y1)) * 2;
}
void GetNeonXY(int pos, int X_LEFT, int Y_TOP, int X_RIGHT, int Y_BOTTOM, int& tx, int& ty)
{
    int width = X_RIGHT - X_LEFT;
    int height = Y_BOTTOM - Y_TOP;
    int pathSize = (width + height) * 2;

    //pos [0, pathSize-1]
    pos = (pos % pathSize + pathSize) % pathSize;

    if (pos < width)//Top
    {
        tx = X_LEFT + pos;
        ty = Y_TOP;
    }
    else if (pos < width + height)//Right
    {
        tx = X_RIGHT;
        ty = Y_TOP + (pos - width);
    }
    else if (pos < width * 2 + height)//Bottom
    {
        tx = X_RIGHT - (pos - (width + height));
        ty = Y_BOTTOM;
    }
    else//Left
    {
        tx = X_LEFT;
        ty = Y_BOTTOM - (pos - (width * 2 + height));
    }
}

//-----Heat Bar-------
void DrawHeatBar()
{
    static int s_lastBarCombo = INT_MIN;
    if (barCombo == s_lastBarCombo && frameCounter % 4 != 0) return;
    s_lastBarCombo = barCombo;

    const int BAR_MAX_H = 25;
    const int BAR_BOTTOM_Y = 25;//BAR_BOTTOM_Y
    const int BAR_X[2] = { 2, 48 };

    float ratio = (float)barCombo / (float)BAR_COMBO_MAX;//rate
    if (ratio > 1.0f) ratio = 1.0f;//When bigger than max just be 1.0f

    //calculate current height
    int baseH = (int)(ratio * BAR_MAX_H);

    for (int side = 0; side < 2; side++)
    {
        int x = BAR_X[side];
        int currentH = 0;

        // Calculate current height of heat bar
        if (barCombo > 0 && barCombo < BAR_COMBO_MAX)
        {
            //ゆらぎ(-1~1)
            float wave = sinf(frameCounter * 0.3f) * 1.5f;
            currentH = (int)(baseH + wave);

            if (currentH < 1) currentH = 1;
            if (currentH > BAR_MAX_H) currentH = BAR_MAX_H;
        }
        else if (barCombo >= BAR_COMBO_MAX)
        {
            currentH = BAR_MAX_H;
        }

        // Draw the bar
        for (int h = 0; h < BAR_MAX_H; h++)
        {
            gotoxy(x, BAR_BOTTOM_Y - h);

            if (h < currentH)
            {
                if (barCombo < BAR_COMBO_MAX)
                {
                    float colorRatio = (float)h / (float)currentH;
                    int idx = (int)(colorRatio * (HEAT_RAMP_SIZE - 1));
                    std::cout << "\x1b[48;5;" << HEAT_RAMP[idx] << "m ";
                }
                else
                {
                    int colorIdx = (h + frameCounter) % RAINBOW_RAMP_SIZE;
                    std::cout << "\x1b[48;5;" << RAINBOW_RAMP[colorIdx] << "m ";
                }
            }
            else
            {
                std::cout << "\x1b[0m ";
            }
        }
        std::cout << "\x1b[0m";
    }
}

//-----Power Gauge ------
void DrawPowerGauge()
{
    //Power Gauge Count
    gaugePer = (float)g_currentScore / scoreGoal;
    if (gaugePer >= 1) gaugePer = 1.0f;
    gaugeCount = (int)(gaugePer * GAUGE_MAX);

    if (gaugeCount == lastGaugeCount) return;
    lastGaugeCount = gaugeCount;

    int colorIdx = (int)(gaugePer * 10);
    if (colorIdx > 10) colorIdx = 10;

    gotoxy(52, 10);
    std::cout << "\x1b[48;5;" << batteryRamp[colorIdx] << "m";
    for (int i = 0; i < gaugeCount; i++)
    {
        std::cout << " ";
    }
    std::cout << "\x1b[0m";
}

//-----Combo Emoji-------
void LoadEmojis()
{
    const char* fnames[7][2] =
    {
        { "EMOJI_0_1.csv", "EMOJI_0_2.csv" },
        { "EMOJI_1_1.csv", "EMOJI_1_2.csv" },
        { "EMOJI_2_1.csv", "EMOJI_2_2.csv" },
        { "EMOJI_3_1.csv", "EMOJI_3_2.csv" },
        { "EMOJI_4_1.csv", "EMOJI_4_2.csv" },
        { "EMOJI_5_1.csv", "EMOJI_5_2.csv" },
        { "EMOJI_6_1.csv", "EMOJI_6_2.csv" },
    };

    for (int s = 0; s < 7; s++)
    {
        for (int f = 0; f < 2; f++)
        {
            std::ifstream file(fnames[s][f]);
            if (!file) continue;

            char ch;
            int row = 0, col = 0;

            while (file.get(ch) && row < 12)
            {
                if (ch == '0' || ch == '1' || ch == '2' || ch == '3' || ch == '4' ||
                    ch == '5' || ch == '6' || ch == '7' || ch == '8' || ch == '9')
                {
                    emojisData[s][f][row][col] = ch - '0';
                    col++;
                    if (col >= 27)
                    {
                        col = 0;
                        row++;
                    }
                }
            }

        }
    }

    for (int s = 0; s < 7; s++)
    {
        for (int f = 0; f < 2; f++)
        {
            for (int y = 0; y < 12; y++)
            {
                std::string rowStr = "";
                rowStr.reserve(256);
                for (int x = 0; x < 27; x++)
                {
                    int colorNum = emojisData[s][f][y][x];
                    switch (colorNum) {
                    case 0: rowStr += "\x1b[48;5;1m "; break;
                    case 1: rowStr += "\x1b[48;5;226m "; break;
                    case 2: rowStr += "\x1b[48;5;208m "; break;
                    case 3: rowStr += "\x1b[48;5;52m ";  break;
                    case 4: rowStr += "\x1b[48;5;255m "; break;
                    case 5: rowStr += "\x1b[48;5;211m "; break;
                    case 6: rowStr += "\x1b[48;5;16m ";  break;
                    case 7: rowStr += "\x1b[48;5;8m ";   break;
                    case 8: rowStr += "\x1b[48;5;33m ";  break;
                    case 9: rowStr += "\x1b[48;5;17m ";  break;
                    default: rowStr += "\x1b[0m ";       break;
                    }
                }
                rowStr += "\x1b[0m";
                precomputedEmojis[s][f][y] = rowStr;
            }
        }
    }
}
void DrawEmojis()
{
    int s = 3;
    if (missCount == 0)
    {
        if (barCombo >= BAR_COMBO_MAX) s = 6;
        else if (barCombo > 60) s = 5;
        else if (barCombo > 30) s = 4;
        else if (barCombo > 0) s = 3;
    }
    else
    {
        if (missCount >= 6) s = 0;
        else if (missCount >= 3) s = 1;
        else s = 2;
    }

    //frame switch
    int f = (frameCounter / 15) % 2;

    if (s == lastEmojiIdx && f == lastFrameIdx) return;
    lastEmojiIdx = s;
    lastFrameIdx = f;

    for (int y = 0; y < 12; y++)
    {
        gotoxy(EMOJI_OFFSET_X, y + EMOJI_OFFSET_Y);
        std::cout << precomputedEmojis[s][f][y];
    }
}

//-----Over Hint-------
void LoadOverHint()
{
    std::ifstream file("OVER.csv");
    if (!file) return;

    char ch;
    int row = 0, col = 0;

    while (file.get(ch) && row < 9)
    {
        if (ch == '0' || ch == '1' || ch == '2')
        {
            overHintData[row][col] = ch - '0';
            col++;
            if (col >= 43)
            {
                col = 0;
                row++;
            }
        }
    }
}
void DrawOverHint()
{
    for (int y = 0; y < 9; y++)
    {
        for (int x = 0; x < 43; x++)
        {
            if (overHintData[y][x] == 0)
            {
                gotoxy(x + 4, y + 8);
                std::cout << "\x1b[48;5;16m ";
            }
            else if (overHintData[y][x] == 1)
            {
                gotoxy(x + 4, y + 8);
                std::cout << "\x1b[48;5;255m ";
            }
            else if (overHintData[y][x] == 2)
            {
                gotoxy(x + 4, y + 8);
                std::cout << "\x1b[48;5;1m ";
            }
        }
    }
    std::cout << "\x1b[0m";
}

//Result Scene
void LoadResult()
{
    std::ifstream file("RESULT.csv");
    if (!file) return;

    char ch;
    int row = 0, col = 0;

    while (file.get(ch) && row < 5)
    {
        if (ch == '0' || ch == '1')
        {
            resultData[row][col] = ch - '0';
            col++;
            if (col >= 56)
            {
                col = 0;
                row++;
            }
        }
    }
}
void DrawResult()
{
    for (int y = 0; y < 5; y++)
    {
        for (int x = 0; x < 56; x++)
        {
            if (resultData[y][x] == 1)
            {
                gotoxy(x + 13, y + 3);
                std::cout << "\x1b[48;5;255m ";
            }
        }
    }
    std::cout << "\x1b[0m";
}
