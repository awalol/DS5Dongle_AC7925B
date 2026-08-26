/*
 * @file lv_example_lyrics_4.c
 *
 * 演示了如何调用 lv_lyrics 控件和蓝牙播歌实现简单的歌词播放 demo
 */

#include "../../lv_examples.h"
//#if LV_USE_LYRICS && LV_BUILD_EXAMPLES
#if 1

#include "os/os_api.h"

#include "lvgl.h"

//#include "lv_lyrics.h"
//#include "lv_lyrics_freetype.h"

#define CONFIG_UI_RES_PATH CONFIG_ROOT_PATH // SD 路径

//#define CONFIG_FONT_TTF_PATH CONFIG_UI_RES_PATH"font/simkai.ttf"    // windos 下完整楷体字库
/*#define CONFIG_FONT_TTF_PATH "mnt/sdfile/res/cfg/OPPO.ttf" // 龘: flash*/
//#define CONFIG_FONT_TTF_PATH "mnt/sdfile/EXT_RESERVED/uipackres/ui/sub1.ttf" // 龘: flash
//#define CONFIG_FONT_TTF_PATH "mnt/sdfile/EXT_RESERVED/uipackres/ui/sub2.ttf"  // 《生僻字部分歌词》: flash


#define CONFIG_FONT_TTF_PATH "storage/sd0/C/OPPO.ttf" // 龘: flash

#define LYRICS_LETTER_NUM_MAX 256
extern lv_obj_t *father_of_curr_obj_0;//父控件
static char lyrics_example4_text[LYRICS_LETTER_NUM_MAX] = "  ";
lv_obj_t *curr_obj_0 = NULL;
lv_obj_t **curr_obj_single = NULL;
int count_of_text = 0;
static char *test_text = "两只老虎跑的快";

//释放歌词
void lyrics_example4_clean(void)
{
    for (int i = 0; i < count_of_text; i++) {
        lv_anim_del(curr_obj_single[i], NULL);
        lv_lyrics_destructor(curr_obj_single[i]);
        curr_obj_single[i] = NULL;
    }
}
//创建歌词
void lv_example_lyrics_4_letter1(lv_obj_t *dest_scr, const char *text, uint16_t font_size, const char *font_file, uint16_t width, uint16_t text_num)
{
    lyrics_example4_clean();

    size_t text_len = strlen(text);
    uint32_t *letter_buf = (uint32_t *)lv_mem_alloc(sizeof(uint32_t) * text_len);    // 空间大小最大不会超过 text_len * 4

    uint16_t letter_num = 0;
    // 解析字符串中的每一个 unicode
    int ofs = 0;
    while (ofs < text_len) {
        uint32_t letter;
        uint32_t letter_next;
        //printf("[chili] %s %d\n", __func__, __LINE__);
        _lv_txt_encoded_letter_next_2(text, &letter, &letter_next, &ofs);
        //printf("ofs = %d; letter = 0x%08x; letter_next = 0x%08x.", ofs, letter, letter_next);

        letter_buf[letter_num] = letter;
        //printf("info.letter_buf[%d] = 0x%08x.", letter_num, letter_buf[letter_num]);

        letter_num++;
        assert(letter_num < LYRICS_LETTER_NUM_MAX);   // 歌词字符数量过大，有需求再修改
    }
    count_of_text = letter_num;
    //printf("letter_num = %d.", letter_num);
    //printf("text_len = %d.",text_len);
    curr_obj_single = (lv_obj_t **)malloc(text_num * sizeof(lv_obj_t *));
    int y = 0;
    int x = 0;
    int xtimes;
    if (font_size <= 18) {
        xtimes = font_size + 3;
    } else {
        xtimes = font_size + font_size / 6;
    }
    for (int i = 0; i < letter_num; i++) {
        if (i == 0) {
            x = 0;
        }
        curr_obj_single[i] = lv_lyrics_create(dest_scr, font_file, font_size, 0, letter_buf + i, 1);
        if (x * xtimes >= width) {
            x = 0;
            y += xtimes;
            lv_lyrics_set_pos(curr_obj_single[i], 0 + x * xtimes, y);
        } else {
            lv_lyrics_set_pos(curr_obj_single[i], 0 + x * xtimes, y);
        }
        x++;
    }
    lv_mem_free(letter_buf);
    return;
}
void lv_example_lyrics_4_letter(lv_obj_t *dest_scr, const char *text, uint16_t font_size, const char *font_file, uint16_t width, uint16_t text_num)
{
    lyrics_example4_clean();

    size_t text_len = strlen(text);
    uint32_t *letter_buf = (uint32_t *)lv_mem_alloc(sizeof(uint32_t) * text_len);    // 空间大小最大不会超过 text_len * 4

    uint16_t letter_num = 0;
    // 解析字符串中的每一个 unicode
    int ofs = 0;
    while (ofs < text_len) {
        uint32_t letter;
        uint32_t letter_next;
        //printf("[chili] %s %d\n", __func__, __LINE__);
        _lv_txt_encoded_letter_next_2(text, &letter, &letter_next, &ofs);
        //printf("ofs = %d; letter = 0x%08x; letter_next = 0x%08x.", ofs, letter, letter_next);

        letter_buf[letter_num] = letter;
        //printf("info.letter_buf[%d] = 0x%08x.", letter_num, letter_buf[letter_num]);

        letter_num++;
        assert(letter_num < LYRICS_LETTER_NUM_MAX);   // 歌词字符数量过大，有需求再修改
    }
    count_of_text = letter_num;
    //printf("letter_num = %d.", letter_num);
    //printf("text_len = %d.",text_len);
    /*
    curr_obj_single = (lv_obj_t **)malloc(text_num*sizeof(lv_obj_t *));
    int y = 0;
    int x = 0;
    int xtimes;
    if (font_size <=18){
        xtimes = font_size + 3;
    }
    else xtimes = font_size + font_size/6;
    for (int i = 0;i<letter_num;i++){
        if (i == 0) x=0;
        curr_obj_single[i] = lv_lyrics_create(dest_scr, font_file, font_size, letter_buf + i, 1);
        if (x*xtimes >= width){
            x = 0;
            y += xtimes;
            lv_lyrics_set_pos(curr_obj_single[i], 0 + x*xtimes, y);
        }
        else lv_lyrics_set_pos(curr_obj_single[i], 0 + x*xtimes, y);
        x++;
    }*/
    father_of_curr_obj_0 = lv_lyrics_create(dest_scr, font_file, font_size, 0, letter_buf, letter_num);
    lv_lyrics_set_pos(father_of_curr_obj_0, 0, 0);
    lv_mem_free(letter_buf);
    return;
}
//更新歌词(外部调用这个即可)
void lv_example_lyrics_4_text_input(void *dest_scr, char *new_text, uint16_t font_size, uint16_t width, uint16_t text_num)
{
    size_t text_len = strlen(new_text);
    if (text_len > LYRICS_LETTER_NUM_MAX) {
        //printf("error: new_text is too len. len = %d.", text_len);
        return;
    }

    //printf("[%s] new lyrics is [%s]", __func__, new_text);

    memcpy(lyrics_example4_text, new_text, text_len);
    lyrics_example4_text[text_len] = '\0';

    lv_example_lyrics_4_letter(dest_scr, lyrics_example4_text, font_size, CONFIG_FONT_TTF_PATH, width, text_num);
}
//调用示例
//在lvgl以外线程使用lvgl_rpc_post_func(),歌词创建在lv_layer_top()页面,test_text为输入歌词
//lvgl_rpc_post_func(lv_example_lyrics_4_text_input, 2, lv_layer_top() , test_text);



#endif