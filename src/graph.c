#if defined(GRAPH)
#include "LabArm.h"


//static const Graph_t NullGraph = {32000,-32000};

typedef struct
{
  int16_t Delta;
  int16_t Intercept;
  int16_t Round0;
  int16_t Round1;
} GraphTable_t;

const GraphTable_t GraphTable [] = {
{30,1,10,5},
{60,2,20,10},
{150,5,50,20},
{300,10,100,50},
{600,20,200,100},
{1500,50,500,200},
{3000,100,1000,500},
{6000,200,2000,1000},
{12000,400,4000,2000},
{24000,800,8000,4000},
};
#define GRAPH_TABLE_SIZE (sizeof(GraphTable)/sizeof(GraphTable_t))

Graph_t IGraphArray[GRAPH_SIZE];
Graph_t VGraphArray[GRAPH_SIZE];
uint8_t GraphCurrentPoint;
GraphData_t GraphData;
int16_t GlobalMin;
int16_t GlobalMax;
uint8_t GlobalMinPoint;
uint8_t GlobalMaxPoint;
uint8_t GlobalDiapason;

void CalculateExtr(int16_t Min, int16_t Max, int16_t* pRoundMin, uint8_t* pDiapason)
{
  int16_t Delta;
  int16_t Round;
  uint8_t Diapason;
  int16_t RoundMin;
  int16_t RoundMax;
  int16_t RoundDelta;

  Delta = Max - Min;
  
  for (Diapason = 0; Diapason<GRAPH_TABLE_SIZE;Diapason++)
  {
    if ( Delta > GraphTable[Diapason].Delta )
      continue;
    break;
  }

ReselectDiapason:  
  RoundDelta = GraphTable[Diapason].Delta;
  Round = GraphTable[Diapason].Round0;

  do
  {
    RoundMin = Min/Round*Round;
    RoundMax = (Max+Round+1)/Round*Round;
    if ( (RoundMax - RoundMin) <= RoundDelta )
      break;
    if (Round != GraphTable[Diapason].Round1)
    {
      Round = GraphTable[Diapason].Round1;
      continue;
    }
    Diapason++;
    goto ReselectDiapason;
  }
  while(1);
  
  {
    int16_t Add = (RoundDelta - (RoundMax - RoundMin))/2;
    RoundMin = RoundMin - Add;
    RoundMax = RoundMax + Add;
    if (RoundMin < 0 )
    {
      RoundMax = RoundMax - RoundMin;
      RoundMin = 0;
    }
  }
  
  Round = GraphTable[Diapason].Round0;
  do
  {
    RoundMin = RoundMin/Round*Round;
    RoundMax = RoundMin + GraphTable[Diapason].Delta;
    if ( RoundMax >= Max && RoundMin <= Min )
      break;
    if (Round != GraphTable[Diapason].Round1)
    {
      Round = GraphTable[Diapason].Round1;
      continue;
    }
    Diapason++;
    goto ReselectDiapason;
  } while(1);

  *pRoundMin = RoundMin;
  *pDiapason = Diapason;
}

static int8_t Offsetmin(Graph_t* Array, int8_t ColumnNum)
{
  return (Array[ColumnNum].Min - GlobalMin)/GraphTable[GlobalDiapason].Intercept+1;
}
static int8_t Offsetmax(Graph_t* Array, int8_t ColumnNum)
{
  return (Array[ColumnNum].Max - GlobalMin)/GraphTable[GlobalDiapason].Intercept+1;
}

void OutOneColumn(Graph_t* Array, int8_t ColumnNum)
{
  int8_t OffsetMin,OffsetMax;
  uint32_t Column = 0;
  int8_t PrevMin;
  int8_t PrevMax;
  int8_t  Prev;
  int8_t  Next;

  Next = GraphCurrentPoint + 1;
  if ( Next >= GRAPH_SIZE)
    Next = 0;

  if( ColumnNum == Next)
  {
    Column = 0xFFFFFFFF;
    goto draw;
  }

  if ( ColumnNum%10 == 0)
  {
    Column = (1<<1)+(1<<3)+(1<<5)+(1<<7)+(1<<9)+(1<<11)+
            (1<<13)+(1<<15)+(1<<17)+(1<<19)+(1<<21)+(1<<23)+(1<<25)+(1<<27)+(1<<29)+(1u<<31);
  }
  if ( ColumnNum%2 == 0)
    Column |= (1<<1)+(1<<11)+(1<<21)+(1u<<31);
  
  Prev = ColumnNum - 1;
  if (Prev < 0)
    Prev = GRAPH_SIZE - 1;

  
  if ( Array[ColumnNum].Min != 32000)
  {
    int8_t Delta;

    OffsetMin = Offsetmin(Array, ColumnNum);
    OffsetMax = Offsetmax(Array, ColumnNum);
    if ( Array[Prev].Min != 32000)
    {
      PrevMin = Offsetmin(Array, Prev);
      PrevMax = Offsetmax(Array, Prev);
    }
    else
    {
      PrevMin = OffsetMin;
      PrevMax = OffsetMax;
    }
    
    Delta = OffsetMax - PrevMax - 1;
    while (Delta > 0)
    {
      Column |= (1<<(OffsetMax-Delta));
      Delta--;
    }

    Delta = PrevMin - OffsetMin - 1;
    while (Delta > 0)
    {
      Column |= (1<<(PrevMin-Delta));
      Delta--;
    }

    Delta = PrevMax - OffsetMax - 1;
    while (Delta > 0)
    {
      Column |= (1<<(PrevMax-Delta));
      Delta--;
    }

    Delta = OffsetMin - PrevMin - 1;
    while (Delta > 0)
    {
      Column |= (1<<(OffsetMin-Delta));
      Delta--;
    }

    Column |= (1<<OffsetMin);
    Column |= (1<<OffsetMax);
  }

draw:
  Column = __RBIT(Column);
  LcdGotoXY(ColumnNum, 5);
  LcdSend(Column>>24, LCD_DATA);
  LcdGotoXY(ColumnNum, 4);
  LcdSend(Column>>16, LCD_DATA);
  LcdGotoXY(ColumnNum, 3);
  LcdSend(Column>>8, LCD_DATA);
  LcdGotoXY(ColumnNum, 2);
  LcdSend(Column, LCD_DATA);
}

void PrintGraph()
{
  Graph_t* Array = GraphData.GraphArray;
  int i;
  uint8_t Diapason;
  int16_t Min=32000;
  int16_t Max=-32000;

  /* Finding extremoum */
  for(i=0; i<GRAPH_SIZE;i++)
  {
    if (Array[i].Max > Max )
    {
      Max = Array[i].Max;
      GlobalMaxPoint = i;
    }
    if(Array[i].Min < Min)
    {
      Min = Array[i].Min;
      GlobalMinPoint = i;
    }
  }
  CalculateExtr(Min, Max, &Min, &Diapason);
  Max = Min + GraphTable[Diapason].Delta;
  if ( GlobalDiapason == Diapason && Min == GlobalMin && Max == GlobalMax)
  {
    int8_t Next = GraphCurrentPoint + 1;
    if ( Next >= GRAPH_SIZE )
      Next = 0;
      
    OutOneColumn(GraphData.GraphArray, Next);
    OutOneColumn(GraphData.GraphArray, GraphCurrentPoint);
    return;
  }
  GlobalMin = Min;
  GlobalMax = Max;
  GlobalDiapason = Diapason;
  for ( i = 0; i<GRAPH_SIZE; i++)
  {
    OutOneColumn(Array, i);
  }
  OutValueSmall(1, 0, GlobalMin, GraphData.DotPosition, 0);
  OutValueSmall(1, 7, GraphTable[GlobalDiapason].Delta/3, GraphData.DotPosition, 0);
}

static void SetupGraph(void);
void DisplayGraph(void)
{
  if (Event == EV_FUNC_FIRST)
  {
    LcdBlank(); /* Clear screen */
    GlobalDiapason = 0xFF;
    PrintGraph();
    return;
  }
  if ( (Event& EV_MASK) == EV_KEY_PRESSED )
  {
    switch (Event & KEY_MASK)
    {
      case KEY_ENTER:
        CurrentFunc(StartFunction);
        return;
      case KEY_DOWN:
      case KEY_UP:
        CurrentFunc(SetupGraph);
        return;
      case KEY_ADC:
      {
        int16_t ValueMin;
        int16_t ValueMax;
        uint8_t Next;


        ValueMin = GraphData.GraphArray[GraphCurrentPoint].Min;
        ValueMax = GraphData.GraphArray[GraphCurrentPoint].Max;
        if (ValueMin < GlobalMin || ValueMax > GlobalMax || /* Larger diapason */
            GraphCurrentPoint == GlobalMaxPoint || GraphCurrentPoint == GlobalMinPoint) /* Check Diapason */
        {
          PrintGraph();
          return;
        }

        Next = GraphCurrentPoint + 1;
        if ( Next >= GRAPH_SIZE)
          Next = 0;

        OutOneColumn(GraphData.GraphArray, GraphCurrentPoint);
        OutOneColumn(GraphData.GraphArray, Next);

        return;
      }
    }
  }
  return;
}

uint16_t TimeInterval = 5; /* 1 second */

static uint8_t IntervalCorrection = 0;
static void SetupGraph(void)
{
  if (Event == EV_FUNC_FIRST)
  {
    LcdBlank(); /* Clear screen */
    MenuPos = 0;
    IntervalCorrection = 0;
    EncCounter = TimeInterval - 1;
    goto redraw;
  }
  if ( (Event& EV_MASK) == EV_KEY_PRESSED )
  {
    switch (Event & KEY_MASK)
    {
      case KEY_UP:
        if (IntervalCorrection == 0)
        {
          MenuPos++;
          if ( MenuPos > 2 )
            MenuPos = 0;
          goto redraw;
        }
      case KEY_DOWN:
        if (IntervalCorrection == 0)
        {
          if ( MenuPos == 0 )
            MenuPos = 2;
          else
            MenuPos--;
          goto redraw;
        }
        TimeInterval = EncCounter + 1;
        goto redraw;

      case KEY_ENTER:
        switch (MenuPos)
        {
          case 1: /* Clear */
            ClearGraph();
          case 0: /* Return */
            CurrentFunc(DisplayGraph);
            return;
          default: /* Interval */
            if ( IntervalCorrection == 0)
              IntervalCorrection = 1;
            else
            {
              IntervalCorrection = 0;
            }
        }
    }
  }      
  return;

redraw:  
  LcdChr(14+1*Y_POSITION+0*X_POSITION + (MenuPos==0?INVERSE:0), "Return");
  LcdChr(14+2*Y_POSITION+0*X_POSITION + (MenuPos==1?INVERSE:0), "Clear");
  LcdChr(14+3*Y_POSITION+0*X_POSITION + (MenuPos==2?INVERSE:0), "Interval");
  OutValueSmall(3, 9, TimeInterval*2, 3, MenuPos==2?1:0);
}

void ClearGraph(void)
{
  int i;

  for (i = 0; i < GRAPH_SIZE; i++)
  {
    IGraphArray[i].Min = VGraphArray[i].Min = 32000;
    IGraphArray[i].Max = VGraphArray[i].Max = -32000;
  }

  GraphCurrentPoint = 0;
  IGraphArray[0].Max = 
      IGraphArray[0].Min = HumanI;
  VGraphArray[0].Max = 
      VGraphArray[0].Min = HumanV;
    
  GlobalMin = 32000;
  GlobalMax = -32000;
}

#endif
