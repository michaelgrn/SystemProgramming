# MyDash

MyDash is a shell designed for Linux x64 architecture, has been run on RedHat OS 7 and Mint OS 20. The functionality of this shell is several fold. The first several iterations were designed to handle recursively calling itself and maintaing a history of commands. As the program grew more complex, signal handling was added; enabling jobs to be running in the background and the foreground while maintaining a list of currently running jobs. Additional features include version control monitoring and logging of errors. 

Before running this program one must make sure to have installed the *readline* library. After this has been installed, one can type 

*make* 

to compile the program and 

*mydash* 

to run the program. If you want to see the version of MyDash being run, then execute

*mydash -v*

In order to clean the directory one can type 

*make clean*
