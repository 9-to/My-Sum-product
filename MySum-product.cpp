#include <bits/stdc++.h>
using namespace std;

static const int MAX_ROW = 2048;
static const int MAX_COLUMN = 2048;
int H[MAX_ROW][MAX_COLUMN];         //検査行列
//***** 変数 *****//
float p = 0.1;                      //通信路誤り率
static const int q = 2;             //次元

float Mcv[MAX_ROW][MAX_COLUMN][q];   //検査ノード->変数ノード
float Mvc[MAX_COLUMN][MAX_ROW][q];   //変数ノード->検査ノード
float g[MAX_COLUMN][q];
vector<vector<int>> I(MAX_ROW);     //変数ノードに接続する検査ノードのインデックス
vector<vector<int>> J(MAX_COLUMN);  //検査ノードに接続する変数ノードのインデックス
int printMatrix(int n , int m);
void printVector(int* V, int length);
int checkMatrixSize(int n, int m, int actN, int actM);
void generateCheckNode(int row , int colmun);
void calcVnode(int row , int column, int y[]);
void calcCnode(int row , int column, int y[]);
float calcProbability(int x, int y);
float innerCalc(int i, int j, int x);
void estimateCalc(int* x,int column, int y[]);
int estimateSymbol(float* g);
int checkSum(int* x, int row, int column);


int main(){
    int n,m;                        //n=行数, m=列数
    cout<<"row= ?, column= ? <- plz edit."<<endl;
    //cin>>n>>m;
    n = 3, m = 7;//確認用
    ifstream file("./matrix.txt");  // 検査行列 H を読み込み
    string line;
    string separator = " ";         //読み込み文字列の分割文字
    int i = 0, j= 0, k = 0;
    int K = 101;                   //最大繰り返し回数
    int y[m];                     //符号語。これを推測してx^を出力する
    cout<<"plz edit y."<<endl;
    for (i=0;i<m;i++){
        cin>>y[i];
    }
    //y[7] = {0, 0, 1, 0, 0, 1, 0};//確認用
    int xHat[m];                    //推定語。これを出力する。

    i = 0;
    while (getline(file, line)) {  // 1行ずつ読み込む
        //cout << line << endl;
        j = 0;
        string line2 = line + separator;
        long l = line2.length(), sl = separator.length();
        string::size_type pos = 0, prev = 0;
        for (;pos < l && (pos = line2.find(separator, pos)) != string::npos; prev = (pos += sl)){
            string item = line2.substr(prev, pos - prev);
            H[i][j] = stoi(item);
            j++;
        }
        i++;
    }
    //行列が正しいかチェック
    int actualN = i, actualM = j;
    if(1 == checkMatrixSize(n,m,actualN,actualM)){
        cout<<"行列が正しくありません"<<endl;
        return 0;
    }
    generateCheckNode(n,m);
    while(k<K){
        calcVnode(n,m,y);
        //printMatrix(n,m);
        calcCnode(n,m,y);
        estimateCalc(xHat,m,y);
        if(checkSum(xHat, n, m) == 0){
            cout<<k<<"回の試行です"<<endl;
            printVector(xHat,m);
            return 0;
        }
        k++;
    }
    cout<<"最大試行です"<<endl;
    printVector(xHat,m);
    return 0;
}

int printMatrix(int n , int m){
    /*
    行列を出力する
    */
    for(int i = 0; i<n; i++){
        for(int j=0; j<m; j++){
            cout << Mvc[j][i][0]<<" ";
        }
        cout<<endl;
    }
    cout<<endl;
    return 0;
}

void printVector(int* V, int length){
    /*
    ベクトルを標準出力する
    */
    for(int i=0; i<length; i++){
            cout<<V[i];
        }
    cout<<endl;
}

int checkMatrixSize(int n, int m, int actN, int actM){
    /*
    n:想定している行
    m:想定している列
    actN:実際の行
    actM:実際の列
    *****
    想定と実際の行列が異なっていたら1を返す
    */
    int check = 0;
    if(n != actN) check = 1;
    if(m != actM) check = 1;
    return check;
}

void generateCheckNode(int row , int column){
    /*
    M c->v を初期化する
    ついでに I, J を計算する
    */
    for(int i=0; i<row; i++){
        for(int j=0; j<column; j++){
            for(int k=0; k<q; k++){
                if(H[i][j] != 0){
                    Mcv[i][j][k] = 1.0;
                }else{
                    Mcv[i][j][k] = 0.0;
                }
            }
            if(H[i][j] != 0){
                J[i].push_back(j);
                I[j].push_back(i);
            }
        }
    }
}

void calcVnode(int row , int column, int y[]){
    /*
    手順2:変数ノード処理を行う
    */
    for(int j=0; j<column; j++){
        for (const auto& i : I[j]) {
            for(int k=0; k<q; k++){
                float tmp = 1;
                for(const auto& ii : I[j]){
                    if(i != ii) tmp *= Mcv[ii][j][k];
                }
                Mvc[j][i][k] = calcProbability(k,y[j])*tmp;
            }
        }
    }
}

void calcCnode(int row , int column, int y[]){
    /*
    手順3:検査ノード処理を行う
    */
   for(int i=0; i<row; i++){
       for (const auto& j : J[i]) {
           for(int k=0; k<q; k++){ // x_j = k
               Mcv[i][j][k] = innerCalc(i,j,k);
           }
       }
   }
}

float innerCalc(int i, int j, int x){        //改修の余地あり　
    /*
    検査ノード処理関数の内部処理
    i:固定した行。検査ノードの番号に合致する。
    j:固定した列。固定した変数の住所に合致する。
    x:固定した変数、つまりx_jの値。
    */
    int len = J[i].size();//インデックスJの要素数
    int chatch = (q - x) % q;//固定した要素以外の求めたい和
    float output = 1.0;
    for(const auto& jj : J[i]){
        if(jj != j){
            for(const auto& jjj : J[i]){
                if(jjj != j && jjj != jj){
                    if(x == 0){
                        output = Mvc[jj][i][0]*Mvc[jjj][i][0]+Mvc[jj][i][1]*Mvc[jjj][i][1];
                    }else{
                        output = Mvc[jj][i][1]*Mvc[jjj][i][0]+Mvc[jj][i][0]*Mvc[jjj][i][1];
                    }
                }
            }
        }
    }
    
    return output;
}

void estimateCalc(int* x, int column, int y[]){
    /*
    手順4:パリティ検査処理
    */
    for(int j=0; j<column; j++){
        for(int k=0; k<q; k++){
            float tmp = 1;
            for(const auto& i : I[j]){
                tmp *= Mcv[i][j][k];
            }
            g[j][k] = tmp * calcProbability(k, y[j]);
        }
        x[j] = estimateSymbol(g[j]);
    }
}

int estimateSymbol(float* g){
    /*
    尤もらしいシンボルを推定する
    */
    int output = 0;
    float most = 0;
    for(int k=0; k<q; k++){
        if(g[k]>=most){
            most = g[k];
            output = k;
        }
    }
    return output;
}

int checkSum(int* x, int row, int column){
    /*
    パリティチェックを行う。Hx^ = 0 ならば 0を返す。
    */
    for(int i=0; i<row; i++){
        int tmp = 0;
        for(int j=0; j<column; j++){
            tmp += H[i][j] * x[j];
        }
        if(tmp%q != 0){
            //cout<<tmp%q<<endl;
            return 1;
        }
    }
    return 0;
}

float calcProbability(int x, int y){
    /*
    確率を計算する
    */
    if(x==y){
        return 1-p;
    }else{
        return p;
    }
}