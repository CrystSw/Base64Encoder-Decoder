/*
Base64エンコーダ・デコーダ
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

/*-----bit-operation-----*/
char get_bitvalue(char*, int);
void set_bitvalue(char*, int, char);

/*-----base64-----*/
char *base64_encode(char*, int);
char *base64_decode(char*, int*);
inline char invconv_table(char);

/*-----option-----*/
inline long get_filesize(char*);

/*
 *ビットインデックスが指すビットの値を取得する
 *0か1しか返さないので、メモリ節約のためにint型ではなくchar型で返す
 *
 *@value plain - 対象のデータ列を指すポインタ
 *@value bit_index - 取得するビットのインデックス 
 *@return - ビット値
 */
char get_bitvalue(char *plain, int bit_index){
	int byte_index = bit_index >> 3;		//文字配列中の対応するインデックスを求める
	char tmp = 0x01;					//ビット評価用変数
	
	//ビット値を返す
	return (plain[byte_index] & (tmp << (7 - (bit_index & 7))) ? 1 : 0);
}

/*
 *ビットインデックスが指すビットに値を格納する
 *
 *@value plain - 対象のデータ列を指すポインタ
 *@value bit_index - 取得するビットのインデックス 
 *@value value - 格納するビット値(0,1)
 */
void set_bitvalue(char *plain, int bit_index, char value){
	int byte_index = bit_index >> 3;		//文字配列中の対応するインデックスを求める
	char tmp = 0x01;					//ビット評価用変数
	
	//bit_indexに従って、任意の1ビットにのみ1を立てた変数を作成
	tmp = tmp << (7 - (bit_index & 7));
	
	if(value == 0){
		//NOT(tmp)と元の値とのANDを取ることで、任意の1ビットに0を立てる
		plain[byte_index] &= ~tmp;
	}else{
		//tmpと元の値とのORを取ることで、任意の1ビットに1を立てる
		plain[byte_index] |= tmp;
	}
}

/*
 *Base64エンコードを行う
 *
 *@value plain - 変換前のデータ列(平文)
 *@value data_size - 変換前のデータ列の長さ
 *@return - Base64文字列
 */
char *base64_encode(char *plain, int data_size){
	
	//変換テーブル
	static const char conv_table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
	
	char bit_data = 0x00;			//一時データ格納用
	int bit_count = 0;				//カウント用
	int bit_length = data_size * 8;	//最大ビット長
	
	char *base64 = (char*)malloc(data_size*4/3+3+1);	//結果格納用の領域
	int wbyte_index = 0;								//書き込むバイトのインデックス
	
	for(int i = 0; i < bit_length; i++) {
		//一時データの2ビット目から順に格納していく
		set_bitvalue(&bit_data, bit_count+2, get_bitvalue(plain, i));
		if(bit_count != 5){
			//6ビット格納するまではインデックスを順にずらしていく
			bit_count++;
		}else{
			//6ビット格納したら、変換テーブルで対応するASCIIコードに変換する
			base64[wbyte_index++] = conv_table[bit_data];
			bit_data = 0x00;
			bit_count = 0;
		}
	}
	//6ビットに満たなかった場合はそのまま出力
	if(bit_count != 0){
		base64[wbyte_index++] = conv_table[bit_data];
	}
	//Base64文字列長が4の倍数になるまで=をつける
	while(wbyte_index & 3 != 0){
		base64[wbyte_index++] = '=';
	}
	//ヌル文字の格納
	base64[wbyte_index] = '\0';
	return base64;
}

/*
 *Base64文字列の逆変換用のテーブル
 *C言語は連想配列や要素からインデックスを取り出す手法が使えないので、こちらで代用
 *
 *@value bit - Base64文字
 */
char invconv_table(char bit){
	if('A' <= bit && bit <= 'Z'){
		bit -= (char)0x41;	//ASCII 'A'-'Z'へ変換
	}else if('a' <= bit && bit <= 'z'){
		bit -= (char)0x47;	//ASCII 'a'-'z'へ変換
	}else if('0' <= bit && bit <= '9'){
		bit += (char)0x04;	//ASCII '0'-'1'へ変換
	}else if(bit == '+'){
		bit = (char)0x3E;		//ASCII '+'へ変換
	}else if(bit == '/'){
		bit = (char)0x3F;		//ASCII '/'へ変換
	}
	return bit;
}

/*
 *Base64デコードを行う
 *
 *@value base64 - Base64文字列
 *@value data_size - デコードしたデータ列の長さを格納するためのポインタ
 *@return 
 */
char *base64_decode(char *base64, int *data_size){
		
	int i = 0;
	
	char bit_data = 0x00;				//一時データ格納用
	int bit_count = 0;					//カウント用
	int byte_length = strlen(base64);	//Base64文字列のバイト長
	int bit_length;						//Base64文字列のビット長
	int plain_length = 0;				//デコード後のサイズ
	
	char *plain = (char*)malloc(byte_length*3/4+1);		//結果格納用の領域
	int wbyte_index = 0;							//書き込むバイトのインデックス
	
	//'='削除
	for(int i = byte_length-1; i >= 0; i--){
		if(base64[i] == '='){
			base64[i] = '\0';
			byte_length--;
		}else{
			break;
		}
	}
	
	//文字長検出
	bit_length = byte_length*8;
	
	//Base64文字を逆変換する
	for(i = 0; i < byte_length; i++)
		base64[i] = invconv_table(base64[i]);
	
	//8ビットずつ切り出す
	for(i = 0; i < bit_length; i++) {
		if((i & 7) <= 1){
			i++;
			continue;
		}
		//一時データに先頭から順番に格納していく
		set_bitvalue(&bit_data, bit_count, get_bitvalue(base64, i));
		if(bit_count != 7){
			//8ビット格納するまではインデックスを順にずらしていく
			bit_count++;
		}else{
			//8ビット格納したら、結果を1バイトとして格納
			plain[wbyte_index++] = bit_data;
			plain_length++;
			bit_data = 0x00;
			bit_count = 0;
		}
	}
	//ヌル文字の格納
	plain[wbyte_index] = '\0';
	
	//デコードしたデータ列の長さを格納
	*data_size = plain_length;
	return plain;
}

/*
 *指定したファイルのファイルサイズを取得する
 *この関数を利用するにはsys/stat.hをインクルードしてください。
 *
 *@value filename - ファイル名
 *@return ファイルサイズ
 */
long get_filesize(char *filename){
	
	struct stat stbuf;
		
	if(stat(filename, &stbuf) == -1)
		return -1L;
		
	return stbuf.st_size;
}

int main(void){
	
	char text[] = "Sample Text\0";
	char *base64 = NULL;
	char *plain = NULL;
	int decode_size;
	
	printf("text:%s\n", text);
	printf("size:%d\n", strlen(text));
	
	base64 = base64_encode(text, strlen(text));
	printf("Encoded text:%s\n", base64);
	printf("size:%d\n", strlen(base64));
	
	plain = base64_decode(base64, &decode_size);
	printf("Decoded text:%s\n", plain);
	printf("size:%d\n", decode_size);
	
	free(base64);
	free(plain);
	
	FILE *fp;
	char *binary = NULL;
	long binary_size;
	
	//画像ファイルを読み込む
	if((fp = fopen("./sample.jpg","rb")) != NULL){
		
		//バイナリサイズの取得
		binary_size = get_filesize("./sample.jpg");
		//データ格納用の領域確保
		binary = (char*)malloc(binary_size);
		//ファイルの中身全部をbinary変数に格納
		fread(binary, sizeof(char), binary_size, fp);
		//base64エンコード
		base64 = base64_encode(binary, binary_size);
		
		printf("Encoded binary:%s\n", base64);
		fclose(fp);
	}
	
	return 0;
}
