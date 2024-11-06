直接用vscode打开这个文件夹就能录入你想要的动作并训练你喜欢的模型<br>
---
训练出来的模型文件是.tflite后缀，需要转成.h才能写进ESP32<br>
在linux系统（或者windows的git bash）运行这段代码将.tflite转成.h：<br>
xxd -i model.tflite > model.h

最后的最后：
===
不要忘了留意我写的注释。<br>
