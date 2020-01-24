#include<iostream>
#include <vector>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <chrono>
#include <climits>
#include <algorithm>

using namespace std;
string trim (string input){ //modify
    int i=0;
    while (i < input.size() && ((input [i] == ' ')))
        i++;
    if (i < input.size())
        input = input.substr (i);
    else{
        return "";
    }

    i = input.size() - 1;
    while (i>=0 && ((input [i] == ' ')))
        i--;
    if (i >= 0)
        input = input.substr (0, i+1);
    else
        return "";

    return input;


}
/*
vector<int> findAllOccurances(string line , char c){
    vector<int> result;
    int found=0;
    found = line.find(c,0);
    while(found != line.size()-1){
        result.push_back(found);
        found = line.find(c,found+1);
    }
    return result;
}
*/
vector<string> split (string line, char separator=' '){ //find ' and " then iterate through and +100 to it to make chars jubberish then undo before returning result of split
    vector<string> result;
    int start =0;
    bool q=0;
    bool c=0;
    for(int i=0;i<line.size();i++){
        if(line[i]=='"'){
            if(!c){
                q = !q;
            }
        }
        if(line[i]=='\''){
            if(!q){
                c = !c;
            }
        }
        if((line[i]==separator) & !(q|c) ){//split
            string sub = line.substr(start,i-start);
            start = i+1;
            sub = trim(sub);
            result.push_back(sub);
        }

    }
    if(start <line.size()){
        result.push_back(trim(line.substr(start,line.size()-start)));
    }
    return result;
    /*vector<string> result;//does not account for things inside "  " or ' '  *******
    while (line.size()){
        size_t found = line.find(separator);
        if (found == string::npos){
            string lastpart = trim (line);
            if (lastpart.size()>0){
                result.push_back(lastpart);
            }
            break;
        }
        string segment = trim (line.substr(0, found));
        //cout << "line: " << line << "found: " << found << endl; //testy testy stuff
        line = line.substr (found+1);

        //cout << "[" << segment << "]"<< endl;
        if (segment.size() != 0)
            result.push_back (segment);


        //cout << line << endl;//more testy testy
    }
    return result;*/
}

char** vec_to_char_array (vector<string> parts){
    char ** result = new char * [parts.size() + 1]; // add 1 for the NULL at the end
    for (int i=0; i<parts.size(); i++){
        // allocate a big enough string
        result [i] = new char [parts [i].size() + 1]; // add 1 for the NULL byte
        strcpy (result [i], parts[i].c_str());
    }
    result [parts.size()] = NULL;
    return result;
}

void execute (string command){
    vector<string> argstrings = split (command, ' ' ); // split the command into space-separated parts //take out ' " w trim
    for(int i=0;i<argstrings.size();i++){
        if((argstrings[i][0]=='"')|(argstrings[i][0]=='\'')){
            argstrings[i] = argstrings[i].substr(1,argstrings[i].size()-2);
        }
    }
    char** args = vec_to_char_array (argstrings);// convert vec<string> into an array of char*
    int err = execvp (args[0], args);
    if(err==-1){
        cout<<strerror(errno)<<endl;
        exit(-1);
    }
}

int main (){
    vector<int> backgroundPID;
    bool longout = 0;
    while (true){ // repeat this loop until the user presses Ctrl + C
        string commandline = "";/*get from STDIN, e.g., "ls  -la |   grep Jul  | grep . | grep .cpp" */
        //put input into command line
        char bufname[30];
        char bufpc[30];
        char filepath[200];
        auto timenow = chrono::system_clock::now();
        getcwd(filepath,200);
        gethostname(bufpc,30);
        if(!longout) {
            cout << cuserid(bufname) << "@" << bufpc << ":" << filepath << "$ ";
        }
        else{
            time_t timetnow = chrono::system_clock::to_time_t(timenow);
            cout << cuserid(bufname) << "@" << bufpc <<"|"<<strtok(ctime(&timetnow),"\n")<< ":" << filepath << "$ ";
        }

        getline(cin, commandline);//*
        //cout<<"after getline \n";
        if(commandline.empty()){
            if(cin.eof()){
                exit(-1);
            }

            if(backgroundPID.size()>0){ //if threre are current background processes
                //do a lot of crying
                for(int & p : backgroundPID){
                    int ended = waitpid(p,&p,WNOHANG);
                    if(ended > 0){//process actually ended
                        remove(backgroundPID.begin(),backgroundPID.end(),ended);
                        cout<<"PID:["<<ended<<"] ended.\n";
                    }
                }
            }
            continue;
        }
        // split the command by the "|", which tells you the pipe levels
        vector<string> tparts = split (commandline, '|');

        //cout<<"before exit check\n";
        if(tparts[0]=="exit"){ //exit hardcoded to exit shell
            return 0;
        }
        //cout<<"after exit check\n";
        if(tparts[0]=="togglelong"){ //Shows terminal prompt with full date and time
            longout = !longout;
            continue;
        }
        vector<string>t0spaces = split(tparts[0],' '); // used to determine if there is a filepath command (seperate frome pipes and normal commands)
        if(t0spaces[0]=="cd"){
            chdir(t0spaces[1].c_str());
            continue;
        }
        //cout<<"========\n";
        // for each pipe, do the following:
        int fdpipe[3];
        for (int i=0; i<tparts.size(); i++){ //pipe file handling
            // make pipe
            fdpipe[2] = fdpipe[0];
            if(i<tparts.size()-1){
                //if not last one
                pipe(fdpipe);
            }
            /*
            if(i > 0){
                //dup2(fdpipe[2], STDIN_FILENO);
                close(fdpipe[1]);
            }
            // */
            int currPID = fork();
            if (!currPID){
                // redirect output to the next level
                //look in tparts[i] for &, > ,or < symbols for redirect
                int foundGT = tparts[i].find('>',0);
                int foundLT = tparts[i].find('<',0);
                int foundAMP = tparts[i].find('&',0);
                if(foundGT != -1){//if it finds output redirection
                    vector<string> outputredir = split(tparts[i],'>');
                    //cout<<"one:"<<outputredir[0]<<"two:"<<outputredir[1]<<endl;
                    int fdo = open(outputredir[1].c_str(),O_WRONLY | O_CREAT | O_TRUNC,0644);
                    dup2(fdo,STDOUT_FILENO);//makes new write file for output then duplicates standard output to it
                    close(fdo);
                    tparts[i]=outputredir[0];
                }
                if(foundLT != -1){//if it finds input redirection
                    vector<string> inputredir = split(tparts[i],'<');
                    int fdi = open(inputredir[1].c_str(), O_RDONLY ,0644);
                    dup2(fdi,STDIN_FILENO);//changes standard input to file
                    close(fdi);
                    tparts[i]=inputredir[0];
                }
                if(foundAMP != -1){
                    //assuming & is always on the end of the command
                    vector<string> ampstring = split(tparts[i],'&');
                    //replace
                    tparts[i]=ampstring[0];
                }
                // unless this is the last level
                if (i < tparts.size() - 1){//here is where you deal with multiple pipes/////////////////////////////////if not last
                    dup2(fdpipe[1], STDOUT_FILENO);
                    close(fdpipe[1]);
                }///////////////////////////////////////////////////////////////////////////////////////////////////////if not first
                //*
                if(i > 0){
                    dup2(fdpipe[2], STDIN_FILENO);
                    close(fdpipe[2]);
                }
                // */
                //execute function that can split the command by spaces to 
                // find out all the arguments, see the definition
                execute (tparts [i]); // this is where you execute
            }else{
                if(tparts[i].find('&',0) != -1){
                    backgroundPID.push_back(currPID);
                    cout<<"PID:["<<currPID<<"] started.\n";
                }
                if(backgroundPID.size()>0){ //if there are current background processes
                    //do a lot of crying
                    for(int& p : backgroundPID){
                        int ended = waitpid(p,&p,WNOHANG);
                        if(ended > 0){//process actually ended
                            remove(backgroundPID.begin(),backgroundPID.end(),ended);
                            cout<<"PID:["<<ended<<"] ended.\n";

                        }
                    }
                }
                if(find(backgroundPID.begin(),backgroundPID.end(),currPID) == backgroundPID.end()){//if current process is not a background process
                    //waitpid(currPID, NULL, 0);// wait for the child process
                    wait(0);
                }
                // then do other redirects//fix alignment ********
                if(i < tparts.size() - 1) {
                    //close(fdpipe[0]);
                    close(fdpipe[1]);
                }
                /*
                if(i < tparts.size()-1) {
                    dup2(fdpipe[0], STDIN_FILENO);
                    close(fdpipe[1]);
                }
                // */
            }
        }

    }
}