libtaomee安装指南

1. 把代码checkout到命名为libtaomee的子目录下。
2. 进入libtaomee子目录，运行INSTALL安装脚本，需要sudo或root权限。
3. 安装完毕。

依赖库
1. libglib2.0以上

libtaomee使用指南

1. 程序示例（a.c）

#include <stdint.h>
#include <stdio.h>

#include <libtaomee/crypt/qdes.h>

int main()
{
    uint32_t in[6] = { 10, 20, 30, 40, 50, 60 };
    uint32_t out[6];

    printf("%d %d %d %d %d %d\n", in[0], in[1], in[2], in[3], in[4], in[5]);
    des_encrypt_n("11122233", in, out, 3);
    in[0] = 1;
    in[1] = 2;
    printf("%d %d %d %d %d %d\n", in[0], in[1], in[2], in[3], in[4], in[5]);
    des_decrypt_n("11122233", out, in, 3);
    printf("%d %d %d %d %d %d\n", in[0], in[1], in[2], in[3], in[4], in[5]);

    return 0;
}

2. 编译

gcc a.c -ltaomee

3. 运行

./a.out 
10 20 30 40 50 60
1 2 30 40 50 60
10 20 30 40 50 60
