#include <iostream>
#include <fstream> /* ifstream class */
#include <string>  /* string class   */
#include <strings.h>
#include <cstdlib> /* atoi   */
#include <sstream> /* istringstream class */
#include <iomanip>
#include <algorithm>

using namespace std;

#define MAXSTOCKAMOUNT 1500

#define ROW 1000
#define COL 10
#define STOCKNUMCOL 1
#define YZPERCENTCOL 8

#define trace   0
#define year_s  2015
#define year_e  2015
#define month_s 1
#define month_e 12
#define day_s   1
#define day_e   31
#define lastday_y 2015
#define lastday_m 3
#define lastday_d 27

#define winsize 30
#define maxbuy_one_stock 10


int countline(char *filename){
    ifstream fin(filename);
    string str_buf;
    int line_stock_history = 0;
    if(fin){
        while(getline(fin, str_buf) != NULL)
            line_stock_history++;
    }
    fin.close();
    fin.clear();
    return line_stock_history;
}

string get_certain_line(char* filename, int line){
    int i;
    ifstream fin(filename);
    string str_buf;

    for(i = 0; i < line; i++){
        getline(fin,str_buf);
    }
    fin.close();
    fin.clear();
    return str_buf;
}

/*New add for New High Price -s*/

typedef struct date_s{
    int y;
    int m;
    int d;
}DATE_T;


typedef struct stockrecord_s{
    DATE_T date;
    float price;
    int volume;
}STOCKRECORD_T;

#define MAX_NAME 20

typedef struct stockinfonow_s{
    char stockname[MAX_NAME];
    char stockname_ch[MAX_NAME];
    DATE_T date;
    int volume;
    float price;
    float PER;  //price earing ratio
}STOCKINFONOW_t;


typedef struct stockinfo_s{
    int ind; //index of stock from StockBaseAddr
    char stockname[20];
    char stockname_ch[20];
    STOCKRECORD_T price_unsort[winsize];
    int ind_oldest; //the previous one of oldest is newest one
    int ind_H_1st;
    STOCKRECORD_T price_sort[winsize];
    unsigned buy_amount;
    STOCKRECORD_T price_buy[maxbuy_one_stock];
    STOCKRECORD_T H_afterbuy;
    float profit;
    int isnotvalid;  //It's the valid bit, which determined the trace of stock is valid or not ,
                     //for the complexity, suppose Not happened in first comming, i.e. need to add stock in first coming
                     //1. If volume someday is low   2. If someday there is no trade

    int isupdate; //Isupdate today
}STOCKINFO_t;


typedef struct ex_dividend_info_s{
    char stockname[20];
    float bef_ex;
    float aft_ex;
}EX_DIVIDEND_INFO_T;


typedef struct total_stockinfo_s{
    STOCKINFO_t *StockBasePtr;
    int stockamount;     //Total kind of stock now
    int windowsize;      //windows size
    float profit;
    float un_gain;       //it's the unrealize gain
    int stockbuyamount;  //Total stocks that owned now
    unsigned int salary; //need implement, to optimize one's inveset strategy
    int exstockamount;
    EX_DIVIDEND_INFO_T exstock[50]; //It's the exclude dividend stock info today
    int islastday;  //flag to determine if today is the last day of our stastitistic => need to sell all of the stocks out
    double MoneyNeed;
    double MaxMoneyNeed;
    unsigned int cnt_Notvalid;
}TOTAL_STOCKINFO_t;

bool pricecompare(STOCKRECORD_T ls, STOCKRECORD_T rs) { return ls.price > rs.price; }

fstream f_out_history("C:\\Works\\20140726YZ\\Gain_history.txt", ios::out | ios::trunc);

void UpdateStockInfo(STOCKINFONOW_t *stock_in, TOTAL_STOCKINFO_t *TotalStockDesc){
    int ind=0, j=0;
    int match=0;
    int ind_oldest, ind_1st;
    int t;
    STOCKINFO_t* StockInfoNow;
    float profit_tmp;
    int rank_volume = 0;
    //cout << "Update Stock Info" <<endl;
    //cout << "stock name in StockInfo " << stock_in->stockname <<endl;
    //cout << "stock ch name in StockInfo " << stock_in->stockname_ch <<endl;
    //cout << "total stock amount " << TotalStockDesc->stockamount <<endl;
    //cout << "TotalStockDesc->StockBasePtr: " << TotalStockDesc->StockBasePtr << endl;
    //cout << "TotalStockDesc->StockBasePtr + 1: " <<TotalStockDesc->StockBasePtr+1 << endl;
    //cout << "size of STOCKINFO_t: " << sizeof(STOCKINFO_t) <<endl;
    //cout << TotalStockDesc->stockamount <<endl;
    //system("PAUSE");
    while(ind < TotalStockDesc->stockamount){
        StockInfoNow = (STOCKINFO_t*)(TotalStockDesc->StockBasePtr + ind);//Get the stock now
        j = 0;
        match = 1;
        while(stock_in->stockname[j] != '\0'){
            if(stock_in->stockname[j] != StockInfoNow->stockname[j]){
                match = 0;
                break;
            }
            j++;
        }
        if(match == 0){
            ind++;
            continue;
        }else{
            StockInfoNow->isupdate = 1;
            if(StockInfoNow->isnotvalid == 1){
                return;
            }
            if((stock_in->volume == 0) || (stock_in->price == -1)){
                // Now the invalid casein only volume = 0
                StockInfoNow->isnotvalid = 1;
                //TotalStockDesc->cnt_Notvalid++;
                if(StockInfoNow->buy_amount){
                    //Kaster
                    StockInfoNow->buy_amount--;
                    TotalStockDesc->stockbuyamount--;
                    TotalStockDesc->MoneyNeed -= StockInfoNow->price_buy[StockInfoNow->buy_amount].price;
                }
                return;
            }
            //... [Old stock] do update of StockInfoNow
            ind_oldest = StockInfoNow->ind_oldest;
            ind_1st = StockInfoNow->ind_H_1st;
            if(StockInfoNow->price_unsort[ind_oldest].price == 0){
                StockInfoNow->price_unsort[ind_oldest].price = stock_in->price;
                StockInfoNow->price_unsort[ind_oldest].date.y = stock_in->date.y;
                StockInfoNow->price_unsort[ind_oldest].date.m = stock_in->date.m;
                StockInfoNow->price_unsort[ind_oldest].date.d = stock_in->date.d;
                StockInfoNow->price_unsort[ind_oldest].volume = stock_in->volume;
                if(StockInfoNow->price_unsort[ind_oldest].price > StockInfoNow->price_unsort[ind_1st].price){
                    ind_1st = StockInfoNow->ind_H_1st = ind_oldest;
                }
            }else{  //overwrite the previous high, resort to get the highest price
                //------refresh the highest price---------
                //Update the oldest price to the newset price
                StockInfoNow->price_unsort[ind_oldest].price = stock_in->price;
                StockInfoNow->price_unsort[ind_oldest].date.y = stock_in->date.y;
                StockInfoNow->price_unsort[ind_oldest].date.m = stock_in->date.m;
                StockInfoNow->price_unsort[ind_oldest].date.d = stock_in->date.d;
                StockInfoNow->price_unsort[ind_oldest].volume = stock_in->volume;


                if(StockInfoNow->buy_amount){
                    // Need to check in every update to find is there any exclude dividend happened.
                    //cout << "ExStock Amount inside: "<< TotalStockDesc->stockamount << " " <<TotalStockDesc->exstockamount << endl
                    match = 0;
                    for(t = 0; t < TotalStockDesc->exstockamount; t++){
                        j = 0;
                        match = 1;
                        while(StockInfoNow->stockname[j] != '\0'){
                            if(StockInfoNow->stockname[j] != TotalStockDesc->exstock[t].stockname[j]){
                                match=0;
                                break;
                            }
                            j++;
                        }
                        if(match == 1)break;

                    }
                    if(match == 1){
                        //cout << "Update the profit by exclude dividend: "<<TotalStockDesc->exstockamount<< " " << StockInfoNow->stockname << " " << TotalStockDesc->exstock[t-1].aft_ex << " " << TotalStockDesc->exstock[t-1].bef_ex << endl;
                        //system("PAUSE");
                        StockInfoNow->profit += (TotalStockDesc->exstock[t].bef_ex - TotalStockDesc->exstock[t].aft_ex);
                        TotalStockDesc->profit += (TotalStockDesc->exstock[t].bef_ex - TotalStockDesc->exstock[t].aft_ex);
                        cout << "\t" << StockInfoNow->stockname << "\t" << StockInfoNow->stockname_ch << "\tExGain\t"
                             << "(" << TotalStockDesc->exstock[t].aft_ex << ","
                             << TotalStockDesc->exstock[t].bef_ex << ")\t"
                             << "(" << StockInfoNow->price_buy[0].date.y
                             << "/" << StockInfoNow->price_buy[0].date.m
                             << "/" << StockInfoNow->price_buy[0].date.d
                             << ","
                             // <<StockInfoNow->price_unsort[ind_oldest].date.y
                             // <<"/"<<StockInfoNow->price_unsort[ind_oldest].date.m
                             // <<"/"<<StockInfoNow->price_unsort[ind_oldest].date.d
                             << ")\t"
                             << "gain:" << (TotalStockDesc->exstock[t].bef_ex - TotalStockDesc->exstock[t].aft_ex)
                             << "\tgain_T: " << TotalStockDesc->profit
                             << endl;
                        f_out_history << "\t" << StockInfoNow->stockname << "\t" << StockInfoNow->stockname_ch << "\tExGain\t"
                                      << "(" << TotalStockDesc->exstock[t].aft_ex << ","
                                      << TotalStockDesc->exstock[t].bef_ex << ")\t"
                                      << "(" << StockInfoNow->price_buy[0].date.y
                                      << "/" << StockInfoNow->price_buy[0].date.m
                                      << "/" << StockInfoNow->price_buy[0].date.d
                                      << ","
                                      // <<StockInfoNow->price_unsort[ind_oldest].date.y
                                      // <<"/"<<StockInfoNow->price_unsort[ind_oldest].date.m
                                      // <<"/"<<StockInfoNow->price_unsort[ind_oldest].date.d
                                      << ")\t"
                                      << "gain:" << (TotalStockDesc->exstock[t].bef_ex - TotalStockDesc->exstock[t].aft_ex)
                                      << "\tgain_T: " <<TotalStockDesc->profit
                                      << endl;
                    }

                    // Refresh the highest price after buy
                    if(StockInfoNow->price_unsort[ind_oldest].price > StockInfoNow->H_afterbuy.price){
                        StockInfoNow->H_afterbuy = StockInfoNow->price_unsort[ind_oldest];
                    }
                    // Check if this is the right time to sell
                    if(StockInfoNow->price_unsort[ind_oldest].price <= StockInfoNow->H_afterbuy.price * 0.93){
                        //If more than one limit down -> sold the stock out

                        /******HERE WE SELL IT OUT!!!******/
                        StockInfoNow->buy_amount--; //Now limited to only one stock
                        TotalStockDesc->stockbuyamount--;
                        TotalStockDesc->MoneyNeed -= StockInfoNow->price_buy[StockInfoNow->buy_amount].price;
                        profit_tmp = StockInfoNow->price_unsort[ind_oldest].price - StockInfoNow->price_buy[StockInfoNow->buy_amount].price;
                        //if((profit_tmp/StockInfoNow->price_buy[StockInfoNow->buy_amount].price)>0.2 || ((profit_tmp*(-1))/StockInfoNow->price_buy[StockInfoNow->buy_amount].price)>0.2){
                            cout << "\t" << StockInfoNow->stockname<<"\t" << StockInfoNow->stockname_ch << "\tSELL\t"
                                 << "(" << StockInfoNow->price_buy[StockInfoNow->buy_amount].price << ","
                                 << StockInfoNow->price_unsort[ind_oldest].price << ")\t"
                                 << "(" << StockInfoNow->price_buy[StockInfoNow->buy_amount].date.y
                                 << "/" << StockInfoNow->price_buy[StockInfoNow->buy_amount].date.m
                                 << "/" << StockInfoNow->price_buy[StockInfoNow->buy_amount].date.d
                                 << ","
                                 << StockInfoNow->price_unsort[ind_oldest].date.y
                                 << "/" << StockInfoNow->price_unsort[ind_oldest].date.m
                                 << "/" << StockInfoNow->price_unsort[ind_oldest].date.d
                                 << ")\t"
                                 << "gain:" << (StockInfoNow->price_unsort[ind_oldest].price - StockInfoNow->price_buy[StockInfoNow->buy_amount].price)
                                 << "\tgain_T: " << TotalStockDesc->profit
                                 << "\tamount_T:" << TotalStockDesc->stockbuyamount
                                 << endl;
                            f_out_history << "\t" << StockInfoNow->stockname << "\t" << StockInfoNow->stockname_ch << "\tSELL\t"
                                          << "(" << StockInfoNow->price_buy[StockInfoNow->buy_amount].price << ","
                                          << StockInfoNow->price_unsort[ind_oldest].price << ")\t"
                                          << "(" << StockInfoNow->price_buy[StockInfoNow->buy_amount].date.y
                                          << "/" << StockInfoNow->price_buy[StockInfoNow->buy_amount].date.m
                                          << "/" << StockInfoNow->price_buy[StockInfoNow->buy_amount].date.d
                                          << ","
                                          << StockInfoNow->price_unsort[ind_oldest].date.y
                                          << "/" << StockInfoNow->price_unsort[ind_oldest].date.m
                                          << "/" << StockInfoNow->price_unsort[ind_oldest].date.d
                                          << ")\t"
                                          << "gain:" << (StockInfoNow->price_unsort[ind_oldest].price - StockInfoNow->price_buy[StockInfoNow->buy_amount].price)
                                          << "\tgain_T: " << TotalStockDesc->profit
                                          << "\tamount_T:" << TotalStockDesc->stockbuyamount
                                          << endl;
                        //}
                        if(StockInfoNow->price_unsort[ind_oldest].price == 0){
                            cout << "Supposed this cannot happened!!" << endl;
                            system("PAUSE");
                        }
                        StockInfoNow->profit += (StockInfoNow->price_unsort[ind_oldest].price - StockInfoNow->price_buy[StockInfoNow->buy_amount].price);
                        TotalStockDesc->profit += (StockInfoNow->price_unsort[ind_oldest].price - StockInfoNow->price_buy[StockInfoNow->buy_amount].price);
                    }

                    // Need to check if it's the last day  => to determine unrealize gain
                    if(StockInfoNow->buy_amount){ //If have't sold it out
                        if(TotalStockDesc->islastday == 1){
                            StockInfoNow->buy_amount--; //Now limited to only one stock
                            TotalStockDesc->stockbuyamount--;
                            TotalStockDesc->MoneyNeed -= StockInfoNow->price_buy[StockInfoNow->buy_amount].price;
                            TotalStockDesc->un_gain += (StockInfoNow->price_unsort[ind_oldest].price - StockInfoNow->price_buy[StockInfoNow->buy_amount].price);
                            cout << "\t" << StockInfoNow->stockname << "\t" << StockInfoNow->stockname_ch << "\tSELL LAST\t"
                                 << "(" << StockInfoNow->price_buy[StockInfoNow->buy_amount].price << ","
                                 << StockInfoNow->price_unsort[ind_oldest].price<<")\t"
                                 << "(" << StockInfoNow->price_buy[StockInfoNow->buy_amount].date.y
                                 << "/" << StockInfoNow->price_buy[StockInfoNow->buy_amount].date.m
                                 << "/" << StockInfoNow->price_buy[StockInfoNow->buy_amount].date.d
                                 << ","
                                 << StockInfoNow->price_unsort[ind_oldest].date.y
                                 << "/" << StockInfoNow->price_unsort[ind_oldest].date.m
                                 << "/" << StockInfoNow->price_unsort[ind_oldest].date.d
                                 << ")\t"
                                 << "un_gain:" << (StockInfoNow->price_unsort[ind_oldest].price - StockInfoNow->price_buy[StockInfoNow->buy_amount].price)
                                 << "\tungain_T: " << TotalStockDesc->un_gain
                                 << "\tamount_T:" << TotalStockDesc->stockbuyamount
                                 << endl;
                            f_out_history    <<"\t"<<StockInfoNow->stockname<<"\t" << StockInfoNow->stockname_ch << "\tSELL LAST\t"
                                     <<"("<<StockInfoNow->price_buy[StockInfoNow->buy_amount].price<<","
                                     <<StockInfoNow->price_unsort[ind_oldest].price<<")\t"
                                     <<"("<<StockInfoNow->price_buy[StockInfoNow->buy_amount].date.y
                                     <<"/"<<StockInfoNow->price_buy[StockInfoNow->buy_amount].date.m
                                     <<"/"<<StockInfoNow->price_buy[StockInfoNow->buy_amount].date.d
                                     <<","
                                     <<StockInfoNow->price_unsort[ind_oldest].date.y
                                     <<"/"<<StockInfoNow->price_unsort[ind_oldest].date.m
                                     <<"/"<<StockInfoNow->price_unsort[ind_oldest].date.d
                                     <<")\t"
                                     <<"un_gain:"<<(StockInfoNow->price_unsort[ind_oldest].price - StockInfoNow->price_buy[StockInfoNow->buy_amount].price)
                                     <<"\tungain_T: " <<TotalStockDesc->un_gain
                                     <<"\tamount_T:" <<TotalStockDesc->stockbuyamount
                                     <<endl;
                            return;
                        }else{
                            //Just count un_gain in every day
                            TotalStockDesc->un_gain += (StockInfoNow->price_unsort[ind_oldest].price - StockInfoNow->price_buy[StockInfoNow->buy_amount-1].price);
                        }
                    }

                    return;
                }

                if(ind_oldest == ind_1st){
                    for(t = 0; t < winsize; t++){
                        StockInfoNow->price_sort[t] = StockInfoNow->price_unsort[t];
                    }
                    std::sort(StockInfoNow->price_sort, StockInfoNow->price_sort+winsize, pricecompare);
                    //Kaster: highest price index find from the oldest  one because there may be more than one candidates
                    for(t = 0; t < winsize; t++){
                        int ind_tmp = (ind_oldest+t)%winsize;
                        if(StockInfoNow->price_sort[0].price == StockInfoNow->price_unsort[ind_tmp].price){
                            ind_1st = StockInfoNow->ind_H_1st = ind_tmp; //Update the 1st index
                            break;
                        }
                        if(t == winsize){
                            cout << "Trouble happened! boom!!!\n";
                            system("PAUSE");
                        }
                    }
                }else{
                    if(StockInfoNow->price_unsort[ind_oldest].price > StockInfoNow->price_unsort[ind_1st].price){
                        ind_1st = StockInfoNow->ind_H_1st = ind_oldest; //Update the 1st index
                        //This is NOT the totally new high price case
                        //(maybe we would delete a highest price and also add a new high price) in the last case
                    }
                }
                //#endif
                //------now oldest is newset, if newest == 1st  => means that new high price----
                if(ind_1st == ind_oldest && TotalStockDesc->islastday!=1){
                    //This is the new high price case!
                    //If it's the lastday => don't need to buy
                    //NEED TO CARE ABOUT HIGHEST PRICE AFTER BUY

                    if(StockInfoNow->buy_amount == 0){
                        //******Kaster: Now we just limit in the case that maximum amount of stock = 1 (per kind of stock)************//
                        for(t = 0; t < winsize; t++){
                            if(t == ind_oldest)continue;
                            //The decision of BUY or NOT depends on the rank of volume
                            if(StockInfoNow->price_unsort[ind_oldest].volume < StockInfoNow->price_unsort[t].volume){
                                rank_volume++;
                            }
                        }
                        if(rank_volume < 3){// && stock_in->PER<12){
                            //Means 0,1,2  ,i.e. top 3
                            StockInfoNow->price_buy[StockInfoNow->buy_amount] = StockInfoNow->price_unsort[ind_oldest];
                            StockInfoNow->H_afterbuy = StockInfoNow->price_unsort[ind_oldest];

                            /******HERE WE BUY IN!!!******/
                            StockInfoNow->buy_amount++;
                            TotalStockDesc->stockbuyamount++;
                            TotalStockDesc->MoneyNeed += StockInfoNow->price_unsort[ind_oldest].price;
                            if(TotalStockDesc->MaxMoneyNeed < TotalStockDesc->MoneyNeed){
                                //cout << "totalmoney needed(bef): " <<TotalStockDesc->MaxMoneyNeed <<endl;
                                TotalStockDesc->MaxMoneyNeed = TotalStockDesc->MoneyNeed;
                                //cout << "totalmoney needed(aft): " <<TotalStockDesc->MaxMoneyNeed <<endl;
                                //system("PAUSE");
                            }
                        }
                    }
                }
            }

            StockInfoNow->ind_oldest = (StockInfoNow->ind_oldest+1)%winsize;

            //StockInfoNow->pidx_unsort = (StockInfoNow->pidx_unsort+1)%winsize;
            return;
        }

    }
    //... [New stock]
    //... Add new stock here, update StockInfoNow[ind]
    cout << "New Stock Comming: ind:" << TotalStockDesc->stockamount <<endl;
    StockInfoNow = (STOCKINFO_t*)(TotalStockDesc->StockBasePtr + TotalStockDesc->stockamount);
#if 0
    cout << "StockInfoNow: " << StockInfoNow << endl;
    cout << &StockInfoNow->ind << endl;
    cout << TotalStockDesc->stockamount << endl;
#endif
    TotalStockDesc->stockamount++;
    StockInfoNow->ind = TotalStockDesc->stockamount;  //bug!!!
    StockInfoNow->isupdate = 1;//Kaster
    t = 0;
    while(stock_in->stockname[t] != '\0'){
        StockInfoNow->stockname[t] = stock_in->stockname[t];
        t++;
    }
    StockInfoNow->stockname[t] = '\0';
    t = 0;
    while(stock_in->stockname_ch[t] != '\0'){
        StockInfoNow->stockname_ch[t] = stock_in->stockname_ch[t];
        t++;
    }
    StockInfoNow->stockname_ch[t] = '\0';
    cout << "Stockname:" << StockInfoNow->stockname_ch <<endl;
    ind_oldest = StockInfoNow->ind_oldest = 0;
    ind_1st = StockInfoNow->ind_H_1st = 0;
    StockInfoNow->price_unsort[ind_oldest].price = stock_in->price;
    StockInfoNow->price_unsort[ind_oldest].date.y = stock_in->date.y;
    StockInfoNow->price_unsort[ind_oldest].date.m = stock_in->date.m;
    StockInfoNow->price_unsort[ind_oldest].date.d = stock_in->date.d;
    StockInfoNow->ind_oldest++;

    if((stock_in->volume == 0) && (stock_in->price == -1)){
        // Now the invalid casein only volume = 0
        StockInfoNow->isnotvalid = 1;
        if(StockInfoNow->buy_amount){
            cout << "Trouble happened!! Supposed no this case!" << endl;
            system("PAUSE");
            TotalStockDesc->MoneyNeed -= StockInfoNow->price_buy[StockInfoNow->buy_amount-1].price;
        }
        //TotalStockDesc->cnt_Notvalid++;
    }
    return;
}

/*New add for New High Price -e*/

int main()
{
    int phase=0;
    char filename[100];
    char filename2[100];
    int y,m,d;
    int i,j,t;
    int ind;

    FILE* f_out_gain = fopen("C:\\Works\\20140726YZ\\Gain_newhigh.csv","w+");
    char out_buffer[100] = "Date,Realized Gain,Unrealized Gain,Money Needed\n";
    fwrite(out_buffer, sizeof(char), sizeof(out_buffer), f_out_gain);

    /*Kaster add for new high price -s*/
    STOCKINFO_t *StockBasePtr = (STOCKINFO_t *)malloc(sizeof(STOCKINFO_t)*MAXSTOCKAMOUNT);
    TOTAL_STOCKINFO_t *TotalStockDesc = (TOTAL_STOCKINFO_t *)malloc(sizeof(TOTAL_STOCKINFO_t));

    cout << "StockBasePtr: " << StockBasePtr <<endl;
    cout << "TotalStockDesc: " << TotalStockDesc <<endl;

    memset(StockBasePtr, 0, sizeof(STOCKINFO_t)*MAXSTOCKAMOUNT);
    memset(TotalStockDesc, 0, sizeof(TOTAL_STOCKINFO_t));

    TotalStockDesc->StockBasePtr = StockBasePtr;

    /*-e*/

    NXTSTOCK:

    STOCKINFONOW_t stock_now;
    TotalStockDesc->stockamount = 0;
    for(y = year_s; y <= year_e; y++){
        for(m = month_s; m <= month_e; m++){
            for(d = day_s; d <= day_e; d++){
                if(m < 10 && d < 10)
                    sprintf(filename, "C:\\Works\\20140726YZ\\stock_all_byday\\A112%d0%d0%dALLBUT0999.csv",y,m,d);
                else if(m < 10)
                    sprintf(filename, "C:\\Works\\20140726YZ\\stock_all_byday\\A112%d0%d%dALLBUT0999.csv",y,m,d);
                else if(d < 10)
                    sprintf(filename, "C:\\Works\\20140726YZ\\stock_all_byday\\A112%d%d0%dALLBUT0999.csv",y,m,d);
                else
                    sprintf(filename, "C:\\Works\\20140726YZ\\stock_all_byday\\A112%d%d%dALLBUT0999.csv",y,m,d);

                if(m < 10 && d < 10)
                    sprintf(filename2, "C:\\Works\\20140726YZ\\exclude_dividend\\TWT49U%d0%d0%d.csv",y,m,d);
                else if(m < 10)
                    sprintf(filename2, "C:\\Works\\20140726YZ\\exclude_dividend\\TWT49U%d0%d%d.csv",y,m,d);
                else if(d < 10)
                    sprintf(filename2, "C:\\Works\\20140726YZ\\exclude_dividend\\TWT49U%d%d0%d.csv",y,m,d);
                else
                    sprintf(filename2, "C:\\Works\\20140726YZ\\exclude_dividend\\TWT49U%d%d%d.csv",y,m,d);
                //cout << filename <<endl;
                //line_file = countline(filename);
                ifstream fin(filename);
                ifstream fin2(filename2);
                string str_buf, str_buf2, str_buf3;
                string str_buf_ex;
                TotalStockDesc->un_gain = 0; //Reset the unrealized gain

                if(!fin) {
                    continue;//cout << "open fail." << endl;
                } else {
                    /******************************************************/
                    /*********open the exclude dividend files-s*****************/
                    /*****************************************************/
                    if(!fin2){
                        //If there is no this file, calldbg
                        cout << "No this file!!" <<endl;
                        system("PAUSE");
                    } else {
                        int linetotal = countline(filename2);
                        int ind_now = 0;
                        //cout << y<<m<<d<<endl;
                        TotalStockDesc->exstockamount = 0;

                        for(t = 0; t < linetotal; t++){
                            if(t == 0 || t == 1){
                                getline(fin2, str_buf_ex);
                                continue;
                            }else if(t == (linetotal-1)){
                                break;
                            }
                            getline(fin2, str_buf_ex);
                            //cout << str_buf_ex <<endl;
                            //system("PAUSE");
                            istringstream stream_ex(str_buf_ex);
                            j = 0;
                            while(getline(stream_ex, str_buf_ex, ',') != NULL){

                                if(j == 0){
                                    //It's the stock number
                                    int ind_frm = 0;
                                    int ind_to = 0;
                                    while(str_buf_ex[ind_frm] != '\0'){

                                        if(str_buf_ex[ind_frm] == '=' || str_buf_ex[ind_frm] == '\"'){
                                            ind_frm++;
                                            continue;
                                        }
                                        TotalStockDesc->exstock[ind_now].stockname[ind_to] = str_buf_ex[ind_frm];
                                        ind_frm++;
                                        ind_to++;
                                    }
                                    TotalStockDesc->exstock[ind_now].stockname[ind_to] = '\0';
                                    //cout<<str_buf_ex<<endl;
                                    //cout<<"Extoday: "<<TotalStockDesc->exstock[ind_now].stockname<<endl;
                                }else if(j == 1){
                                    //stock number in chinese, no need
                                }else if(j == 2){
                                    //It's the price before exclude dividend
                                    TotalStockDesc->exstock[ind_now].bef_ex = atof(str_buf_ex.c_str());
                                    //cout<<str_buf_ex<<endl;
                                    //cout<<TotalStockDesc->exstock[ind_now].bef_ex<<endl;
                                }else if(j == 3){
                                    //It's the price after exclude dividend
                                    TotalStockDesc->exstock[ind_now].aft_ex = atof(str_buf_ex.c_str());
                                    //cout<<str_buf_ex<<endl;
                                    //cout<<TotalStockDesc->exstock[ind_now].aft_ex<<endl;
                                }

                                j++;
                            }
                            ind_now++;
                            TotalStockDesc->exstockamount++;
                            //cout<<"Extockamount++: "<<TotalStockDesc->exstockamount<<endl;
                        }
                        //cout<<"Extockamount last: "<<TotalStockDesc->exstockamount<<endl;
                        fin2.close();
                        fin2.clear();
                    }

                    /******************************************************/
                    /*********open the exclude dividend files-e*****************/
                    /*****************************************************/

                    phase = 0;
                    memset(&stock_now, 0, sizeof(STOCKINFONOW_t)); //initialize
                    if(y == lastday_y && m == lastday_m && d == lastday_d){
                        TotalStockDesc->islastday = 1;
                    }else{
                        TotalStockDesc->islastday = 0;
                    }
                    //bzero(&stock_now, sizeof(STOCKINFONOW_t)); //initialize
                    while(getline(fin, str_buf) != NULL){
                        //cout << str_buf << endl;
                        istringstream stream_ori(str_buf); // for determin phase[0,1,2] using
                        istringstream stream(str_buf);

                        if(phase == 0){
                            //cout << "phase 0";
                            getline(stream_ori, str_buf, ',');
                            if(str_buf[0] == '='){
                                phase = 1; //means that we find 0050, and goto phase 1
                                goto PHASE1;
                            }else
                               continue;
                        }else if(phase == 1){
                            getline(stream_ori, str_buf, ',');
                            if(str_buf[0] != '='){
                                phase = 2; //means that we find 1101, and goto phase 2
                                goto PHASE2;
                            }
PHASE1:
                            j = 0;
                            while(getline(stream, str_buf, '\"') != NULL){
                                if(j == 1){
                                    //eg, we got "0050 "
                                    istringstream stream2(str_buf);
                                    getline(stream2, str_buf2, ' ');//then we got "0050" (without space)
                                    //Stock name, type 0050
#if trace == 1
                                    cout << "stock number: ";
                                    cout << str_buf2;
                                    cout << endl;
                                    //system("PAUSE");
#endif
                                    t = 0;
                                    while(str_buf2[t] != '\0'){
                                        stock_now.stockname[t] = str_buf2[t];
                                        t++;
                                    }
                                    stock_now.stockname[t] = '\0';
                                }else if(j == 3){
                                    //eg, we got "¥xÆW50"
                                    t = 0;
                                    while(str_buf[t] != '\0'){
                                        stock_now.stockname_ch[t] = str_buf[t];
                                        t++;
                                    }
                                    stock_now.stockname_ch[t] = '\0';
#if trace == 1
                                    cout << "stock chinese name: ";
                                    cout << str_buf;
                                    cout << endl;
                                    //system("PAUSE");
#endif
                                }else if(j == 5){
                                    //eg, we got "volume"
                                    stock_now.volume = atoi(str_buf.c_str());
#if trace == 1
                                    cout << "volume: ";
                                    cout << str_buf;
                                    cout << endl;
                                    //system("PAUSE");
#endif
                                }else if(j == 17){
                                    //eg, we got "price"
                                    stock_now.price = atof(str_buf.c_str());
                                    if(str_buf.c_str()[0] == '-'){
                                        //It's the incorrect case, price set to -1, and will be invalid this stock when updatestockinfo
                                        stock_now.price = -1;
                                    }
#if trace == 1
                                    cout << "volume: ";
                                    cout << str_buf;
                                    cout << endl;
                                    //system("PAUSE");
#endif
                                }else if(j == 31){
                                    //eg, we got PER
                                    stock_now.PER = atof(str_buf.c_str());
#if trace == 1
                                    cout << "PER: ";
                                    cout << str_buf;
                                    cout << endl;
                                    //system("PAUSE");
#endif
                                }
                                j++;
                            }
                            stock_now.date.y = y;
                            stock_now.date.m = m;
                            stock_now.date.d = d;
                            UpdateStockInfo(&stock_now,TotalStockDesc);
                        }
                        else if(phase == 2){
PHASE2:
                            j = 0;
                            while(getline(stream, str_buf, '\"') != NULL){

                                if(j == 1){
                                    //eg, we got "1101 "
                                    istringstream stream2(str_buf);
                                    getline(stream2, str_buf2, ' ');//then we got "1101" (without space)
                                    //Stock name, type 1101
#if trace == 1
                                    cout << "stock number: ";
                                    cout << str_buf2;
                                    cout << endl;
                                    //system("PAUSE");
#endif
                                    t = 0;
                                    while(str_buf2[t] != '\0'){
                                        stock_now.stockname[t] = str_buf2[t];
                                        t++;
                                    }
                                    stock_now.stockname[t] = '\0';
                                }else if(j == 3){
                                    //eg, we got "¥xªd"
                                    t = 0;
                                    while(str_buf[t] != '\0'){
                                        stock_now.stockname_ch[t] = str_buf[t];
                                        t++;
                                    }
                                    stock_now.stockname_ch[t] = '\0';
#if trace == 1
                                    cout << "stock chinese name: ";
                                    cout << str_buf;
                                    cout << endl;
                                    //system("PAUSE");
#endif
                                }else if(j == 5){
                                    //eg, we got "volume"
                                    stock_now.volume = atoi(str_buf.c_str());
#if trace == 1
                                    cout << "volume: ";
                                    cout << str_buf;
                                    cout << endl;
                                    //system("PAUSE");
#endif
                                }else if(j == 17){
                                    //eg, we got "price"
                                    stock_now.price = atof(str_buf.c_str());
                                    if(str_buf.c_str()[0] == '-'){
                                        //It's the incorrect case, price set to -1, and will be invalid this stock when updatestockinfo
                                        stock_now.price = -1;
                                    }
#if trace == 1
                                    cout << "price: ";
                                    cout << str_buf;
                                    cout << endl;
                                    //system("PAUSE");
#endif
                                }else if(j == 31){
                                    //eg, we got PER
                                    stock_now.PER = atof(str_buf.c_str());
#if trace == 1
                                    cout << "PER: ";
                                    cout << str_buf;
                                    cout << endl;
                                    //system("PAUSE");
#endif
                                }
                                j++;
                            }
                            if(j != 32){
                                break; //20150215:The end of all stocks, need to go to next day
                            }
                            stock_now.date.y = y;
                            stock_now.date.m = m;
                            stock_now.date.d = d;
                            UpdateStockInfo(&stock_now,TotalStockDesc);
                        }
                    }

                    //Kaster: Change to sell the invalid stock everyday
                    //Sell the invalid stock
                    ind = 0;
                    STOCKINFO_t* StockInfoNow;

                    while(ind<TotalStockDesc->stockamount){
                        StockInfoNow = (STOCKINFO_t*)(TotalStockDesc->StockBasePtr + ind);//Get the stock now
                        if(StockInfoNow->buy_amount && StockInfoNow->isupdate == 0){
                            StockInfoNow->buy_amount--;
                            TotalStockDesc->stockbuyamount--;
                            TotalStockDesc->MoneyNeed -= StockInfoNow->price_buy[StockInfoNow->buy_amount].price;
                            cout << "\t" << StockInfoNow->stockname<<"\t" << StockInfoNow->stockname_ch << "\tINVALID SELL\t"
                                 << "(" << StockInfoNow->price_buy[StockInfoNow->buy_amount].price << ","
                                 << StockInfoNow->price_buy[StockInfoNow->buy_amount].price << ")\t"
                                 << "(" << StockInfoNow->price_buy[StockInfoNow->buy_amount].date.y
                                 << "/" << StockInfoNow->price_buy[StockInfoNow->buy_amount].date.m
                                 << "/" << StockInfoNow->price_buy[StockInfoNow->buy_amount].date.d
                                 << ","
                                 << y
                                 << "/" << m
                                 << "/" << d
                                 << ")\t"
                                 << "un_gain: 0"
                                 << "\tungain_T: " << TotalStockDesc->un_gain
                                 << "\tamount_T:" << TotalStockDesc->stockbuyamount
                                 << endl;

                            cout << "Stock: " << StockInfoNow->stockname << " Buy Price " << StockInfoNow->price_buy[0].price
                                 << " Date " << StockInfoNow->price_buy[0].date.y << StockInfoNow->price_buy[0].date.m << StockInfoNow->price_buy[0].date.d <<endl;

                            f_out_history << "\t" << StockInfoNow->stockname<<"\t" << StockInfoNow->stockname_ch << "\tINVALID SELL\t"
                                          << "(" << StockInfoNow->price_buy[StockInfoNow->buy_amount].price << ","
                                          << StockInfoNow->price_buy[StockInfoNow->buy_amount].price << ")\t"
                                          << "(" << StockInfoNow->price_buy[StockInfoNow->buy_amount].date.y
                                          << "/" << StockInfoNow->price_buy[StockInfoNow->buy_amount].date.m
                                          << "/" << StockInfoNow->price_buy[StockInfoNow->buy_amount].date.d
                                          << ","
                                          << y
                                          << "/" << m
                                          << "/" << d
                                          << ")\t"
                                          << "un_gain: 0"
                                          << "\tungain_T: " << TotalStockDesc->un_gain
                                          << "\tamount_T:" << TotalStockDesc->stockbuyamount
                                          << endl;

                            f_out_history << "Stock: " << StockInfoNow->stockname << " Buy Price " << StockInfoNow->price_buy[0].price
                                          << " Date " << StockInfoNow->price_buy[0].date.y << StockInfoNow->price_buy[0].date.m << StockInfoNow->price_buy[0].date.d <<endl;
                        }
                        StockInfoNow->isupdate = 0;
                        ind++;
                    }

                    if(TotalStockDesc->islastday == 1){
                        //Sell the invalid stock
                        TotalStockDesc->cnt_Notvalid = 0;
                        ind = 0;
                        STOCKINFO_t* StockInfoNow;
                        while(ind<TotalStockDesc->stockamount){
                            StockInfoNow = (STOCKINFO_t*)(TotalStockDesc->StockBasePtr + ind);//Get the stock now
                            if(StockInfoNow->buy_amount){
                                StockInfoNow->buy_amount--;
                                TotalStockDesc->stockbuyamount--;
                                TotalStockDesc->MoneyNeed -= StockInfoNow->price_buy[StockInfoNow->buy_amount].price;
                                cout << "\t" << StockInfoNow->stockname<<"\t" << StockInfoNow->stockname_ch << "\tSELL LAST\t"
                                     << "(" << StockInfoNow->price_buy[StockInfoNow->buy_amount].price << ","
                                     << StockInfoNow->price_buy[StockInfoNow->buy_amount].price << ")\t"
                                     << "(" << StockInfoNow->price_buy[StockInfoNow->buy_amount].date.y
                                     << "/" << StockInfoNow->price_buy[StockInfoNow->buy_amount].date.m
                                     << "/" << StockInfoNow->price_buy[StockInfoNow->buy_amount].date.d
                                     << ","
                                     << y
                                     << "/" << m
                                     << "/" << d
                                     << ")\t"
                                     << "un_gain: 0"
                                     << "\tungain_T: " << TotalStockDesc->un_gain
                                     << "\tamount_T:" << TotalStockDesc->stockbuyamount
                                     << endl;
                                TotalStockDesc->cnt_Notvalid++;
                                cout << "Stock: " << StockInfoNow->stockname << " Buy Price " << StockInfoNow->price_buy[0].price
                                     << " Date " << StockInfoNow->price_buy[0].date.y << StockInfoNow->price_buy[0].date.m << StockInfoNow->price_buy[0].date.d <<endl;
                            }
                            ind++;
                        }
                    }
                    memset(out_buffer,0,sizeof(out_buffer)/sizeof(char));
                    sprintf(out_buffer, "%d/%d/%d,%f,%f,%f\n",y,m,d,TotalStockDesc->profit, TotalStockDesc->un_gain, TotalStockDesc->MoneyNeed);
                    fwrite(out_buffer, sizeof(char), sizeof(out_buffer), f_out_gain);
                    fin.close();
                    fin.clear();
                }

                if(TotalStockDesc->islastday == 1)break;
            }
        }
    }
    cout << "TotalStock gain: " << TotalStockDesc->profit << endl;
    cout << "Max Money needed: " << TotalStockDesc->MaxMoneyNeed << endl;
    cout << "Invalid stock amount: " << TotalStockDesc->cnt_Notvalid << endl;

END:

    fclose(f_out_gain);
    f_out_history.close();
    f_out_history.clear();
    free(StockBasePtr);
    free(TotalStockDesc);
    system("PAUSE");
    return 0;
}
