# Windows-Custom-command-line-shell
I implemented a command line shell that does10 features such as auto-completion, pipelining, multithreading, background processes, job control, built in commands, output and input redirection, file support configuration, signal handling and basic commands
How to run this code and test each features
first open windows command prompt an go to the diretorywhere the file is located
Type gcc -o Library library.c then typ the following command library.exe to get into the shell
To test basic command features type echo hello world
To test auto completion or autosuggest type suggest pw it will suggest pwd and print out the current directory
To test auto completion or autosuggest type suggest hi it will suggest history and print out all the previous commands inputed
next to test for background procesess, go ahed and add many jobs by typing commands such as ping yahoo.com &, ping localhost -n 10 &, Ping gmail.com & and may more. it should run all jobs
you can check if jobs were added sucesfulyy typing the command jobs. this wil list all jobs added
Now we test job control by typing the commands fg [num]. num stands for the job number that yu would like to bring to the forground. to kill a job jst type te command kill[num]. to check if both the comands of fg and kill works you can type the command jobs to list curentjobs and the jobs that have been killed or foreground should not be in the list.
To test output redirection, simply type the command echo hello world! > big_output.txt. then type dir you should see the file big_output.txt with the message.
To test input redirection, choose a file you already have in your directory i will choose a random file called check. simply type the command < check.txt and it should print the content of the file.
To test pipelining, simply type the command echo hello world! | find "Hello" it should print hello world
To test pipelining futher, simply type the command dir | find "txt" it should show all txt files
To test multithreading, simply type the command copy check.txt copiedfile.txt. this should copy all content in check.txt to copiedfile.txt. 
To test file support config, simply type the command load_config config.txt. make sure to have a file called config.txt
getenv setting1
