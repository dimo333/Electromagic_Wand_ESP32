1、你需要先把get_mpu6050_data的代码烧录进esp。<br>
2、打开tensorflow_model_train里面的serial_data文件。（我推荐这一步使用vscode，同时不要忘记关掉上一步烧录esp用到的arduino ide,如果不关可能会占用串口导致vscode无法打开）<br>
3、右键-选择在终端中运行python，这样你就能在按下按键后去录你想要做的动作。<br>
4、最终文件都录好之后打开train文件，右键-选择在终端中运行python。稍等片刻就会生成一个.tflite文件，你需要在命令行输入指令使其变为.h文件。<br>
5、最后把.h文件拖进和wand文件同级的文件夹内。<br>
至此，这就是魔杖最基础的训练流程了。<br>
