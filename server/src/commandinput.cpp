#include "totype.hpp"
#include "flwrapper.hpp"
#include "monoserver.hpp"
#include "threadpool.hpp"
#include "commandinput.hpp"
#include "commandwindow.hpp"

extern MonoServer *g_monoServer;
int CommandInput::handle(int event)
{
    switch(event){
        case FL_KEYBOARD:
            {
                if(!active()){
                    // to inform fltk that we have handled this event
                    return 1;
                }

                const std::string currCmdStr = value() ? value() : "";
                switch(const auto key = Fl::event_key()){
                    case FL_Up:
                    case FL_Down:
                        {
                            if(currCmdStr.empty() || (m_inputListPos >= 0 && m_inputListPos < to_d(m_inputList.size()))){
                                m_inputListPos += ((key == FL_Up) ? -1 : 1);
                                if(m_inputListPos < 0){
                                    // stop at first line if push many UP key
                                    m_inputListPos = 0;
                                }
                                else if(m_inputListPos > to_d(m_inputList.size())){
                                    // stop at last blank line if push many DOWN key
                                    m_inputListPos = to_d(m_inputList.size());
                                }

                                if(m_inputListPos >= 0 && m_inputListPos < to_d(m_inputList.size())){
                                    value(m_inputList.at(m_inputListPos).c_str());
                                }
                                else{
                                    value("");
                                }

                                // to inform fltk that we have handled this event
                                return 1;
                            }

                            else{
                                return Fl_Multiline_Input::handle(event);
                            }
                        }
                    case FL_Enter:
                        {
                            if(currCmdStr.empty()){
                                // to inform fltk that we have handled this event
                                return 1;
                            }

                            // if last char is escape as ``\"
                            // don't commit the command for execution
                            if(true && !currCmdStr.empty()
                                    &&  currCmdStr.back() == '\\'){
                                return Fl_Multiline_Input::handle(event);
                            }

                            value("");
                            m_inputList.push_back(currCmdStr);
                            m_inputListPos = to_d(m_inputList.size());

                            if(true && m_window
                                    && m_window->getLuaModule()){

                                // print current command for echo
                                // should give method to disable the echo

                                // won't use CommandWindow::AddLog() directly
                                // use MonoServer::AddCWLog() for thread-safe access
                                const int cwid = m_window->getLuaModule()->CWID();

                                // we echo the command to the command window
                                // enter in command string will be commit to lua machine
                                // but for echo we need to remove it

                                auto fnGetPrompt = [first = true]() mutable
                                {
                                    if(first){
                                        first = false;
                                        return "$ ";
                                    }
                                    else{
                                        return "> ";
                                    }
                                };

                                size_t currLoc = 0;
                                while(currLoc < currCmdStr.size()){
                                    const auto enterLoc = currCmdStr.find_first_of('\n', currLoc);
                                    if(true
                                            && (enterLoc >= currLoc)
                                            && (enterLoc != std::string::npos)){

                                        // we do find an enter
                                        // remove the enter and print it
                                        g_monoServer->addCWLogString(cwid, 0, fnGetPrompt(), currCmdStr.substr(currLoc, enterLoc - currLoc).c_str());
                                        currLoc = enterLoc + 1;
                                    }
                                    else{
                                        // can't find a enter
                                        // we done here for the whole string
                                        g_monoServer->addCWLogString(cwid, 0, fnGetPrompt(), currCmdStr.substr(currLoc).c_str());
                                        break;
                                    }
                                }

                                // put a task in the thread pool
                                // and return immediately for current thread, don't block GUI logic

                                deactivate();
                                m_worker->addTask([this, cwid, currCmdStr](int)
                                {
                                    try{
                                        if(const auto callResult = m_window->getLuaModule()->execRawString(currCmdStr.c_str()); callResult.valid()){
                                            // default nothing printed
                                            // can put information here to show call succeeds
                                        }
                                        else{
                                            sol::error err = callResult;
                                            std::stringstream errStream(err.what());

                                            // need to handle here
                                            // error message could be more than one line

                                            std::string errLine;
                                            while(std::getline(errStream, errLine, '\n')){
                                                g_monoServer->addCWLogString(cwid, 2, ">>> ", errLine.c_str());
                                            }
                                        }
                                    }
                                    catch(const std::exception &e){
                                        g_monoServer->addCWLogString(cwid, 2, ">>> ", e.what());
                                    }
                                    catch(...){
                                        g_monoServer->addCWLogString(cwid, 2, ">>> ", "unknown error");
                                    }

                                    // need to protect any FLTK widget access by Fl::lock() and Fl::unlock()
                                    // check FLTK manual for multithreading: https://www.fltk.org/doc-1.3/advanced.html
                                    fl_wrapper::mtcall([this]()
                                    {
                                        activate();
                                        m_window->redrawAll();
                                    });
                                });

                                // to inform fltk that we have handled this event
                                return 1;
                            }
                            break;
                        }
                    default:
                        {
                            break;
                        }
                }
            }
        default:
            {
                break;
            }

    }
    return Fl_Multiline_Input::handle(event);
}
