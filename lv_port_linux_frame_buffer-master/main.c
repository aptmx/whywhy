#include "lvgl/lvgl.h"
#include "lvgl/demos/lv_demos.h"
#include "lv_drivers/display/fbdev.h"
#include "lv_drivers/indev/evdev.h"
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <strings.h>

#define DISP_BUF_SIZE (128 * 1024)

// 静态变量
static lv_obj_t * tab_cata;
static lv_obj_t * tab_fruits;
static lv_obj_t * tab_snacks;
static lv_obj_t * tab_drinks;
static lv_obj_t * tab_search;
static lv_obj_t * theme1;
static lv_obj_t * theme2;
static lv_obj_t * tabview;
static int height;
static int width;
static lv_obj_t * spinbox1;
static lv_obj_t * btn_purchase;
static lv_obj_t * keyboard_search;
static lv_obj_t * text_area;
static lv_obj_t * product_info;
static const char * filename = "/root/product.txt";

static void show_product_info(lv_event_t * e);
static void search_item(lv_event_t * e);

// 收货机商品信息节点构建
typedef struct List
{
    int ID;            // 商品编号
    int cat;           // 商品种类
    char Name[32];     // 商品名称
    float Price;       // 商品单价
    char PicPath[128]; // 图片路径
    int Remain;        // 余量

    struct List * next;
    struct List * prev;
} LL;

static LL * Head;

// 初始化商品链表头节点
LL * List_Init(void)
{
    LL * Head = malloc(sizeof(LL));
    if(Head == NULL) {
        perror("malloc");
        return NULL;
    }

    Head->next = Head;
    Head->prev = Head;

    return Head; // 返回头指针
}

// 添加商品信息
void list_TailInsert(LL * Head, LL Data)
{
    LL * NewNode = malloc(sizeof(LL));

    // 数据域
    strcpy(NewNode->Name, Data.Name);
    strcpy(NewNode->PicPath, Data.PicPath);
    NewNode->ID     = Data.ID;
    NewNode->cat    = Data.cat;
    NewNode->Price  = Data.Price;
    NewNode->Remain = Data.Remain;

    // 指针域
    LL * p        = Head->prev;
    Head->prev    = NewNode;
    NewNode->next = Head;
    NewNode->prev = p;
    p->next       = NewNode;
}

// demo显示tab的scroll
static void scroll_begin_event(lv_event_t * e)
{
    /*Disable the scroll animations. Triggered when a tab button is clicked */
    if(lv_event_get_code(e) == LV_EVENT_SCROLL_BEGIN) {
        lv_anim_t * a = lv_event_get_param(e);
        if(a) a->time = 0;
    }
}

// 添加按钮 购买数量和更改总价函数
static void btn1_inc(lv_event_t * e)
{
    if(((LL *)(e->user_data))->Remain > 0) {
        lv_spinbox_increment(spinbox1);

        int n = lv_spinbox_get_value(spinbox1);
        if(n <= ((LL *)(e->user_data))->Remain) {
            printf("n:%d\n", n);

            lv_obj_clean(btn_purchase);
            lv_obj_t * label_p = lv_label_create(btn_purchase);
            float pp           = ((LL *)(e->user_data))->Price;
            printf("%.2f\n", pp);
            float sum_p = pp * n;
            lv_label_set_text_fmt(label_p, "Total:%.2f", sum_p);
        } else {
            lv_obj_clean(btn_purchase);
            lv_obj_t * label_p = lv_label_create(btn_purchase);
            float pp           = ((LL *)(e->user_data))->Price;
            printf("%.2f\n", pp);
            float sum_p = pp * (((LL *)(e->user_data))->Remain);
            lv_spinbox_set_value(spinbox1, ((LL *)(e->user_data))->Remain);
            lv_label_set_text_fmt(label_p, "Total:%.2f", sum_p);
        }
    } else {
        lv_obj_t * target = lv_event_get_current_target(e); // 关闭按钮
        lv_obj_add_state(target, LV_STATE_DISABLED);
    }
}

// 减少按钮 购买数量和更改总价函数
static void btn2_dec(lv_event_t * e)
{
    lv_spinbox_decrement(spinbox1);

    int n = lv_spinbox_get_value(spinbox1);
    printf("n:%d\n", n);

    if(n == 0) {
        lv_obj_t * target = lv_event_get_current_target(e); // 关闭按钮
        lv_obj_add_state(target, LV_STATE_DISABLED);
    }

    lv_obj_clean(btn_purchase);
    lv_obj_t * label_p = lv_label_create(btn_purchase);
    float pp           = ((LL *)(e->user_data))->Price;
    printf("%.2f\n", pp);
    float sum_p = pp * n;
    lv_label_set_text_fmt(label_p, "Total:%.2f", sum_p);
}

// 生成订单
static int generate_order(lv_event_t * e, int n, float sum)
{
    // 打开文件
    FILE * fp;
    if(access("order.txt", F_OK))
        fp = fopen("order.txt", "w+");
    else
        fp = fopen("order.txt", "a+");
    
    // 获取时间
    time_t t = time(NULL);
    char * s = ctime(&t);

    // 获取行号
    FILE * fp_line;
    int h = 0;
    if(access("line", F_OK)) 
    {
        fp_line = fopen("line", "w+");
        int m   = 0;
        fwrite(&m, 4, 1, fp_line);
        fflush(fp_line);
        printf("创建\n");
    } 
    else
    {
        fp_line = fopen("line", "r+");
        fread(&h, 4, 1, fp_line);
    }
    h++;

    // 写入订单文件 order.txt
    fprintf(fp, "%d\tID: %d\tName:%s\tBought:%d\tPayment:%.2f\tTime:%s\n", h, ((LL *)(e->user_data))->ID,
            ((LL *)(e->user_data))->Name, n, sum, s);
    fflush(fp);

    fseek(fp_line, 0, SEEK_SET);
    fwrite(&h, 4, 1, fp_line);
    printf("h:%d\n", h);
    fflush(fp_line);

    fclose(fp);
    fclose(fp_line);
    return 0;
}

// 重新生成商品信息 product.txt
static void reload_product_txt(void)
{
    FILE * fp_prod = fopen(filename, "w+");
    LL * p1        = Head->next;
    for(; p1 != Head; p1 = p1->next) {
        printf("LL Data%d = {%d, %d, %s, %.2f, %s, %d}\n", p1->ID, p1->ID, p1->cat, p1->Name, p1->Price, p1->PicPath,
               p1->Remain);
        fprintf(fp_prod, "ID:%d, Cat:%d, Name:%s, Price:%.2f, Path:%s, Remain:%d;\n", p1->ID, p1->cat, p1->Name,
                p1->Price, p1->PicPath, p1->Remain);
        fflush(fp_prod);
    }
    fclose(fp_prod);
}

// 购买确认弹窗并更改数据 回调函数
static void msg_event(lv_event_t * e)
{
    lv_obj_t * target = lv_event_get_current_target(e);
    printf("confirm\n");

    if(lv_msgbox_get_active_btn(target) == 0) // 获取按钮是哪个
    {
        // 获得购买数量并调整库存
        int n = lv_spinbox_get_value(spinbox1);
        ((LL *)(e->user_data))->Remain -= n;
        printf("confim:n-%d\n", n);

        reload_product_txt(); // reload 商品信息

        // 生成订单
        float p   = ((LL *)(e->user_data))->Price;
        float sum = p * n;
        generate_order(e, n, sum);

        // 重置购买数量
        lv_spinbox_set_value(spinbox1, 0);
        lv_obj_clean(btn_purchase);

        n = lv_spinbox_get_value(spinbox1);
        printf("n:%d\n", n);

        lv_obj_t * label_p = lv_label_create(btn_purchase);
        float pp           = ((LL *)(e->user_data))->Price;
        printf("%.2f\n", pp);
        float sum_p = pp * n;
        lv_label_set_text_fmt(label_p, "T:%.2f", sum_p);
        lv_msgbox_close(target);

        show_product_info(e);
        printf("showinfo\n");

    } else
        lv_msgbox_close(target);
}

// 支付对话框 回调函数
static void pay_sum(lv_event_t * e)
{
    // 创建按钮与消息框
    static const char * btns[] = {"Confirm", "Cancel", ""};
    lv_obj_t * msgbox          = lv_msgbox_create(NULL, "Purshasing...", "Do you continue to buy?", btns, true);
    lv_obj_set_size(msgbox, 300, 200);
    lv_obj_center(msgbox);

    lv_obj_add_event_cb(msgbox, msg_event, LV_EVENT_VALUE_CHANGED, e->user_data);
}

// 显示某个商品详情信息回调函数
static void show_product_info(lv_event_t * e)
{
    // 获取当前时间类型
    lv_event_code_t t = lv_event_get_code(e);
    // item = lv_event_get_current_target(e);
    printf("product2\n");

    // 判断类型
    if(t == 1 | 2) // LV_EVENT_PRESSED
    {
        // 创建商品详情信息画布
        product_info = lv_obj_create(lv_scr_act());
        lv_obj_set_size(product_info, 0.27 * width, 0.7 * height);
        lv_obj_set_flex_flow(product_info, LV_FLEX_FLOW_COLUMN_WRAP_REVERSE);
        lv_obj_align_to(product_info, tabview, LV_ALIGN_OUT_RIGHT_MID, 5, 0);
        lv_obj_clear_flag(product_info, LV_OBJ_FLAG_SCROLLABLE);

        // 添加商品信息到画布上
        lv_obj_t * img1 = lv_img_create(product_info); // 图片
        lv_img_set_src(img1, ((LL *)(e->user_data))->PicPath);
        lv_img_set_zoom(img1, 300);

        lv_obj_t * label02 = lv_label_create(product_info); // 名字
        lv_label_set_text_fmt(label02, "Name: %s", ((LL *)(e->user_data))->Name);

        lv_obj_t * label03 = lv_label_create(product_info); // 价格
        lv_label_set_text_fmt(label03, "Price: RMB %.2f", ((LL *)(e->user_data))->Price);

        lv_obj_t * label04 = lv_label_create(product_info); // 剩余数量
        lv_label_set_text_fmt(label04, "Left: %d item(s)", ((LL *)(e->user_data))->Remain);

        /*购买*/
        // 显示数量
        spinbox1 = lv_spinbox_create(product_info);
        lv_spinbox_set_step(spinbox1, 1);      // 步长
        lv_spinbox_set_range(spinbox1, 0, 10); // 范围

        lv_spinbox_set_value(spinbox1, 0); // 购买初值

        lv_spinbox_set_digit_format(spinbox1, 2, 0);
        lv_obj_set_size(spinbox1, 0.1 * width, 0.1 * height);
        // lv_spinbox_set_pos(spinbox, 0);

        // 购买按钮
        // 减少数量
        lv_obj_t * btn2 = lv_btn_create(product_info);
        // lv_obj_align_to(btn2,spinbox1,LV_ALIGN_OUT_LEFT_MID,-5,0);

        lv_obj_t * label_dec = lv_label_create(btn2);
        lv_label_set_text(label_dec, "-");

        // 增减数量
        lv_obj_t * btn1 = lv_btn_create(product_info);
        // lv_obj_align_to(btn1,spinbox1,LV_ALIGN_OUT_RIGHT_MID,5,0);

        lv_obj_t * label_add = lv_label_create(btn1);
        lv_label_set_text(label_add, "+");

        // 结算
        btn_purchase              = lv_btn_create(product_info);
        lv_obj_t * label_purchase = lv_label_create(btn_purchase);

        int num   = lv_spinbox_get_value(spinbox1);
        float sum = num * (((LL *)(e->user_data))->Price);
        printf("sum: %d\n", sum);
        lv_label_set_text_fmt(label_purchase, "Total: %.2f", sum);

        lv_obj_add_event_cb(btn1, btn1_inc, LV_EVENT_PRESSED, e->user_data);
        lv_obj_add_event_cb(btn2, btn2_dec, LV_EVENT_PRESSED, e->user_data);

        // 支付并改变数据
        lv_obj_add_event_cb(btn_purchase, pay_sum, LV_EVENT_PRESSED, e->user_data);
    }
}

// 显示所有商品函数
static void show_all_item(lv_event_t * e)
{
    LL * p = Head->next;
    lv_obj_set_flex_flow(tab_cata, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_clean(tab_cata);
    for(; p != Head; p = p->next) {
        lv_obj_t * prod = lv_obj_create(tab_cata);
        lv_obj_clear_flag(prod, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_size(prod, 80, 80);

        lv_obj_t * img = lv_img_create(prod);
        lv_obj_set_align(img, LV_ALIGN_CENTER);
        lv_img_set_src(img, p->PicPath);

        lv_obj_add_event_cb(prod, show_product_info, LV_EVENT_PRESSED, (void *)p);
    }
}

// 按种类显示商品 tabview
static void show_cata_info(lv_event_t * e)
{
    // 获得事件目标
    lv_obj_t * target = lv_event_get_current_target(e);

    if(target == tab_cata) // 全种类
    {
        show_all_item(e);
    } else if(target == tab_fruits) // 显示水果
    {
        lv_obj_clean(tab_fruits);
        lv_obj_set_flex_flow(tab_fruits, LV_FLEX_FLOW_ROW_WRAP);
        LL * p = Head->next;
        for(; p != Head; p = p->next) {
            if(p->cat == 2) {
                lv_obj_t * prod = lv_obj_create(tab_fruits);
                lv_obj_set_size(prod, 80, 80);
                lv_obj_clear_flag(prod, LV_OBJ_FLAG_SCROLLABLE);

                lv_obj_t * img = lv_img_create(prod);
                lv_obj_set_align(img, LV_ALIGN_CENTER);
                lv_img_set_src(img, p->PicPath);

                lv_obj_add_event_cb(prod, show_product_info, LV_EVENT_PRESSED, (void *)p);
            }
        }
    } else if(target == tab_snacks) // 显示小吃
    {
        lv_obj_clean(tab_snacks);
        lv_obj_set_flex_flow(tab_snacks, LV_FLEX_FLOW_ROW_WRAP);
        LL * p = Head->next;
        for(; p != Head; p = p->next) {
            if(p->cat == 3) {
                lv_obj_t * prod = lv_obj_create(tab_snacks);
                lv_obj_set_size(prod, 80, 80);
                lv_obj_clear_flag(prod, LV_OBJ_FLAG_SCROLLABLE);

                lv_obj_t * img = lv_img_create(prod);
                lv_obj_set_align(img, LV_ALIGN_CENTER);
                lv_img_set_src(img, p->PicPath);

                lv_obj_add_event_cb(prod, show_product_info, LV_EVENT_PRESSED, (void *)p);
            }
        }
        printf("tab_snacks\n");
    } else if(target == tab_drinks) // 显示饮品
    {
        lv_obj_clean(tab_drinks);
        lv_obj_set_flex_flow(tab_drinks, LV_FLEX_FLOW_ROW_WRAP);
        LL * p = Head->next;
        for(; p != Head; p = p->next) {
            if(p->cat == 4) {
                lv_obj_t * prod = lv_obj_create(tab_drinks);
                lv_obj_set_size(prod, 80, 80);
                lv_obj_clear_flag(prod, LV_OBJ_FLAG_SCROLLABLE);

                lv_obj_t * img = lv_img_create(prod);
                lv_obj_set_align(img, LV_ALIGN_CENTER);
                lv_img_set_src(img, p->PicPath);
                printf("product1\n");
                lv_obj_add_event_cb(prod, show_product_info, LV_EVENT_PRESSED, (void *)p);
            }
        }
        printf("tab_drinks\n");
    }
}

// 按种类显示商品 tabs
static void show_tab_item(lv_event_t * e)
{
    lv_obj_t * target = lv_event_get_target(e);
    uint16_t id       = lv_btnmatrix_get_selected_btn(target);
    printf("btn:%d\n", id);
    char * s = lv_btnmatrix_get_btn_text(target, id);
    printf("btntext:%s\n", s);

    if(!(strcmp(s, "Catalog"))) // 全种类
    {
        show_all_item(e);
    } else if(!(strcmp(s, "Fruits"))) // 显示水果
    {
        lv_obj_clean(tab_fruits);
        lv_obj_set_flex_flow(tab_fruits, LV_FLEX_FLOW_ROW_WRAP);
        LL * p = Head->next;
        for(; p != Head; p = p->next) {
            if(p->cat == 2) {
                lv_obj_t * prod = lv_obj_create(tab_fruits);
                lv_obj_set_size(prod, 80, 80);
                lv_obj_clear_flag(prod, LV_OBJ_FLAG_SCROLLABLE);

                lv_obj_t * img = lv_img_create(prod);
                lv_obj_set_align(img, LV_ALIGN_CENTER);
                lv_img_set_src(img, p->PicPath);

                lv_obj_add_event_cb(prod, show_product_info, LV_EVENT_PRESSED, (void *)p);
            }
        }
    } else if(!(strcmp(s, "Snacks"))) // 显示小吃
    {
        lv_obj_clean(tab_snacks);
        lv_obj_set_flex_flow(tab_snacks, LV_FLEX_FLOW_ROW_WRAP);
        LL * p = Head->next;
        for(; p != Head; p = p->next) {
            if(p->cat == 3) {
                lv_obj_t * prod = lv_obj_create(tab_snacks);
                lv_obj_set_size(prod, 80, 80);
                lv_obj_clear_flag(prod, LV_OBJ_FLAG_SCROLLABLE);

                lv_obj_t * img = lv_img_create(prod);
                lv_obj_set_align(img, LV_ALIGN_CENTER);
                lv_img_set_src(img, p->PicPath);

                lv_obj_add_event_cb(prod, show_product_info, LV_EVENT_PRESSED, (void *)p);
            }
        }
        printf("tab_snacks\n");
    } else if(!(strcmp(s, "Drinks"))) // 显示饮品
    {
        lv_obj_clean(tab_drinks);
        lv_obj_set_flex_flow(tab_drinks, LV_FLEX_FLOW_ROW_WRAP);
        LL * p = Head->next;
        for(; p != Head; p = p->next) {
            if(p->cat == 4) {
                lv_obj_t * prod = lv_obj_create(tab_drinks);
                lv_obj_set_size(prod, 80, 80);
                lv_obj_clear_flag(prod, LV_OBJ_FLAG_SCROLLABLE);

                lv_obj_t * img = lv_img_create(prod);
                lv_obj_set_align(img, LV_ALIGN_CENTER);
                lv_img_set_src(img, p->PicPath);
                printf("product1\n");
                lv_obj_add_event_cb(prod, show_product_info, LV_EVENT_PRESSED, (void *)p);
            }
        }
        printf("tab_drinks\n");
    } else if(!(strcmp(s, "Search"))) // Search
    {
        search_item(e);
    }
}

static void search_item(lv_event_t * e)
{
    // lv_obj_clean(tab_search);

    lv_obj_t * target    = lv_event_get_current_target(e);
    lv_event_code_t code = lv_event_get_code(e);

    if(code == LV_EVENT_FOCUSED) {

        lv_obj_clear_flag(keyboard_search, LV_OBJ_FLAG_HIDDEN);
    }

    if(code == LV_EVENT_VALUE_CHANGED) {
        const char * txt = lv_textarea_get_text(target); // 获取文本
        printf("txt:%s\n", txt);

        lv_obj_clean(tab_search);
        lv_obj_set_flex_flow(tab_search, LV_FLEX_FLOW_ROW_WRAP);

        printf("goon\n");
        lv_obj_clean(tab_search);
        lv_obj_set_flex_flow(tab_search, LV_FLEX_FLOW_ROW_WRAP);
        LL * p = Head->next;
        for(; p != Head; p = p->next) {
            if(strstr(p->Name, txt) != NULL) {
                lv_obj_t * prod = lv_obj_create(tab_search);
                lv_obj_set_size(prod, 80, 80);
                lv_obj_clear_flag(prod, LV_OBJ_FLAG_SCROLLABLE);

                lv_obj_t * img = lv_img_create(prod);
                lv_obj_set_align(img, LV_ALIGN_CENTER);
                lv_img_set_src(img, p->PicPath);
                printf("product_search\n");
                lv_obj_add_event_cb(prod, show_product_info, LV_EVENT_PRESSED, (void *)p);
            }
        }
        printf("tab_search\n");
    }

    if(code == LV_EVENT_DEFOCUSED) {
        printf("defocused\n");
        lv_obj_add_flag(keyboard_search, LV_OBJ_FLAG_HIDDEN); // 隐藏键盘
    }
}

int main(void)
{
    /*LittlevGL init*/
    // 初始化LVGL
    lv_init();

    /*Linux frame buffer device init*/
    fbdev_init();

    /*A small buffer for LittlevGL to draw the screen's content*/
    static lv_color_t buf[DISP_BUF_SIZE];

    /*Initialize a descriptor for the buffer*/
    static lv_disp_draw_buf_t disp_buf;
    lv_disp_draw_buf_init(&disp_buf, buf, NULL, DISP_BUF_SIZE);

    /*Initialize and register a display driver*/
    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.draw_buf = &disp_buf;
    disp_drv.flush_cb = fbdev_flush;
    disp_drv.hor_res  = 800;
    disp_drv.ver_res  = 480;
    lv_disp_drv_register(&disp_drv);

    evdev_init();
    static lv_indev_drv_t indev_drv_1;
    lv_indev_drv_init(&indev_drv_1); /*Basic initialization*/
    indev_drv_1.type = LV_INDEV_TYPE_POINTER;

    /*This function will be called periodically (by the library) to get the mouse position and state*/
    indev_drv_1.read_cb      = evdev_read;
    lv_indev_t * mouse_indev = lv_indev_drv_register(&indev_drv_1);

    /*Set a cursor for the mouse*/ // 设置屏幕鼠标
    // LV_IMG_DECLARE(mouse_cursor_icon)
    // lv_obj_t * cursor_obj = lv_img_create(lv_scr_act()); /*Create an image object for the cursor */
    // lv_img_set_src(cursor_obj, &mouse_cursor_icon);           /*Set the image source*/
    // lv_indev_set_cursor(mouse_indev, cursor_obj);             /*Connect the image  object to the driver*/

    /*Create a Usercase 用例--自动售卖机*/

    // 初始化链表
    Head = List_Init();

    // 打开商品文件
    FILE * file = fopen(filename, "r+");
    if(file == NULL) {
        perror("open() failed");
        exit(0);
    }

    // 读取文件，并写入链表中
    char line[1024];
    while(fgets(line, sizeof(line), file)) {
        LL Data;
        int k;
        // printf("%s", line);
        sscanf(line, "ID:%d, Cat:%d, Name:%[^,], Price:%f, Path:%[^,], Remain:%d;", &Data.ID, &(Data.cat), Data.Name,
               &(Data.Price), Data.PicPath, &(Data.Remain));

        // printf("ID:%d Name:%s Price:%.2f, Path:%s\n", Data.ID, Data.Name,Data.Price,Data.PicPath);

        list_TailInsert(Head, Data);
    }
    fclose(file);

    // 创建全屏画布
    lv_obj_t * main = lv_obj_create(lv_scr_act());
    width           = lv_obj_get_width(lv_scr_act());
    height          = lv_obj_get_height(lv_scr_act());

    lv_obj_set_size(main, width, height);

    // 主画布flex分布
    lv_obj_set_flex_flow(main, LV_FLEX_FLOW_ROW_WRAP);

    /*顶部区域*/
    // 顶部标题
    theme1 = lv_obj_create(main);
    lv_obj_set_size(theme1, 0.92 * width, 0.15 * height);

    lv_obj_t * label = lv_label_create(theme1); // 顶部区域
    lv_obj_set_style_bg_color(label, lv_color_hex(0x00FF00), LV_STATE_DEFAULT);
    lv_obj_set_align(label, LV_ALIGN_CENTER);
    lv_obj_clear_flag(theme1, LV_OBJ_FLAG_SCROLLABLE);

    lv_label_set_text(label, "Vending Machine"); // 小卖铺名称

    // 搜索框设置
    text_area = lv_textarea_create(theme1);
    lv_obj_set_size(text_area, 70, 20);
    lv_obj_set_align(text_area, LV_ALIGN_RIGHT_MID);

    lv_textarea_set_cursor_pos(text_area, LV_TEXTAREA_CURSOR_LAST); // 游标
    lv_textarea_set_one_line(text_area, true);                      // 单行模式
    lv_textarea_set_max_length(text_area, 15);                      // 字符长度

    lv_textarea_set_placeholder_text(text_area, "search"); // 占位符

    // keyboard设置
    keyboard_search = lv_keyboard_create(lv_scr_act());
    lv_obj_set_size(keyboard_search, width * 0.4, height * 0.2);
    lv_obj_align(keyboard_search, LV_ALIGN_TOP_LEFT, 5, 5);
    lv_keyboard_set_mode(keyboard_search, LV_KEYBOARD_MODE_TEXT_LOWER);

    lv_keyboard_set_textarea(keyboard_search, text_area);

    lv_obj_add_event_cb(text_area, search_item, LV_EVENT_ALL, NULL);
    lv_obj_add_flag(keyboard_search, LV_OBJ_FLAG_HIDDEN);

    /*侧边目录+商品显示*/
    // 创建tabview
    tabview = lv_tabview_create(main, LV_DIR_LEFT, 100);
    lv_obj_set_size(tabview, 0.65 * width, 0.7 * height);

    // tabview滚动与渲染
    // lv_obj_add_event_cb(lv_tabview_get_content(tabview), scroll_begin_event, LV_EVENT_SCROLL_BEGIN, NULL);

    // lv_obj_set_style_bg_color(tab_btns, lv_palette_darken(LV_PALETTE_CYAN, 2), 0);
    // lv_obj_set_style_text_color(tab_btns, lv_palette_lighten(LV_PALETTE_GREY, 18), 0);
    // lv_obj_set_style_border_side(tab_btns, LV_BORDER_SIDE_RIGHT, LV_PART_ITEMS | LV_STATE_CHECKED);
    // lv_obj_set_style_bg_color(tabview, lv_palette_lighten(LV_PALETTE_LIGHT_GREEN, 2), 0);

    // tabs 设置不同的tabview
    tab_cata   = lv_tabview_add_tab(tabview, "Catalog");
    tab_fruits = lv_tabview_add_tab(tabview, "Fruits");
    tab_snacks = lv_tabview_add_tab(tabview, "Snacks");
    tab_drinks = lv_tabview_add_tab(tabview, "Drinks");

    tab_search = lv_tabview_add_tab(tabview, "Search");

    lv_obj_t * tab_btns = lv_tabview_get_tab_btns(tabview); // 获取按钮组件

    show_all_item(NULL);
    lv_obj_add_event_cb(tab_cata, show_cata_info, LV_EVENT_PRESSED, NULL); // tabview
    lv_obj_add_event_cb(tab_fruits, show_cata_info, LV_EVENT_PRESSED, NULL);
    lv_obj_add_event_cb(tab_snacks, show_cata_info, LV_EVENT_PRESSED, NULL);
    lv_obj_add_event_cb(tab_drinks, show_cata_info, LV_EVENT_PRESSED, NULL);

    lv_obj_add_event_cb(tab_btns, show_tab_item, LV_EVENT_PRESSED, NULL); // tab_btns

    /*广告*/
    // 广告位
    product_info = lv_obj_create(lv_scr_act());
    lv_obj_set_size(product_info, 0.27 * width, 0.7 * height);
    lv_obj_set_flex_flow(product_info, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_align_to(product_info, tabview, LV_ALIGN_OUT_RIGHT_MID, 5, 0);

    lv_obj_t * img_ad1  = lv_img_create(product_info);
    lv_obj_t * img_ad2  = lv_img_create(product_info);
    lv_obj_t * img_ad3  = lv_img_create(product_info);
    lv_obj_t * img_ad4  = lv_img_create(product_info);
    lv_obj_t * img_ad5  = lv_img_create(product_info);
    lv_obj_t * img_ad6  = lv_img_create(product_info);
    lv_obj_t * img_ad7  = lv_img_create(product_info);
    lv_obj_t * img_ad8  = lv_img_create(product_info);
    lv_obj_t * img_ad9  = lv_img_create(product_info);
    lv_obj_t * img_ad10 = lv_img_create(product_info);
    lv_obj_t * img_ad11 = lv_img_create(product_info);

    lv_img_set_src(img_ad1, "S:/root/product/pic/ad.png");
    lv_img_set_src(img_ad2, "S:/root/product/pic/tuiguang.png");
    lv_img_set_src(img_ad3, "S:/root/product/pic/nongfu.png");
    lv_img_set_src(img_ad4, "S:/root/product/pic/ping.png");
    lv_img_set_src(img_ad5, "S:/root/product/pic/jing.png");
    lv_img_set_src(img_ad6, "S:/root/product/pic/kang.png");
    lv_img_set_src(img_ad7, "S:/root/product/pic/bank.png");
    lv_img_set_src(img_ad8, "S:/root/product/pic/yue.png");
    lv_img_set_src(img_ad9, "S:/root/product/pic/zhu.png");
    lv_img_set_src(img_ad10, "S:/root/product/pic/ying.png");
    lv_img_set_src(img_ad11, "S:/root/product/pic/zhao.png");

    lv_img_set_zoom(img_ad11, 300);

    /*Handle LitlevGL tasks (tickless mode)*/
    while(1) {
        lv_timer_handler(); // LVGL事务处理函数，读坐标，遍历对象链表，看哪个对象被操作
        usleep(5000);       // 每5ms调用一次
    }

    return 0;
}

/*Set in lv_conf.h as `LV_TICK_CUSTOM_SYS_TIME_EXPR`*/
uint32_t custom_tick_get(void)
{
    static uint64_t start_ms = 0;
    if(start_ms == 0) {
        struct timeval tv_start;
        gettimeofday(&tv_start, NULL);
        start_ms = (tv_start.tv_sec * 1000000 + tv_start.tv_usec) / 1000;
    }

    struct timeval tv_now;
    gettimeofday(&tv_now, NULL);
    uint64_t now_ms;
    now_ms = (tv_now.tv_sec * 1000000 + tv_now.tv_usec) / 1000;

    uint32_t time_ms = now_ms - start_ms;
    return time_ms;
}
