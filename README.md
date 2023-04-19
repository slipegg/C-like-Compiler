# C-like-Compiler

使用C++编写的类C语法编译器，支持词法分析、语法分析和语义分析。

详情请见三份报告，这里展示自己构建的较为复杂的程序的分析结果。

## 分析结果

自己构建的较为复杂的程序：

```cpp
//test
int fun(int i,int j)
{
/*
There is something need to delete;
233333
*/
    if(i<j*j+100/2)
    {
        i+=1;
        while(i>j)
        {
            i=2;
            j+=i;
        }
    }
    return;
}

int  program(int a,int b,int c)
{
    int i;
    int j;
    i+=-a+b*2234e-2+i*(j+i/23.4)-100;
    for(int i=j;i<100;i=i+1)
    {
      c+=2*(i+1);
      if(i+j*8>233)
      {
         int i;
         j=i;
      }
      else
      {
          i+=2;
      }
    }
    i=i+1;
    return i;       
}
```

语义分析结果：

![image](https://user-images.githubusercontent.com/65942634/233097622-4431d290-6482-4d70-988c-953409d07b48.png)

![image](https://user-images.githubusercontent.com/65942634/233097661-0a506e95-a34e-47a1-9595-1c2cf34b10b1.png)

![image](https://user-images.githubusercontent.com/65942634/233097911-427422d6-4967-4498-9318-09517fdf0284.png)


