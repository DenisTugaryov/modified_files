#include <iostream>
#include <set>
#include <map>
#include <utility>
#include <algorithm>
#include <csignal>
#include <cerrno>

#include <sys/inotify.h>

#include "boost/filesystem/operations.hpp"
#include "boost/filesystem/path.hpp"


namespace fs = boost::filesystem;

const unsigned int MASK = IN_CLOSE_WRITE; // "Writtable file was closed"

std::map<int, fs::path> wd_to_file_path; // It maps watch descriptor to corresponding file path
std::set<fs::path> modified_file;
int fd = 0; // file descriptor


void print_file_path(const fs::path & file_path)
{
  std::cout<< file_path << std::endl;
}


void rm_watch(const std::pair<int, fs::path> & pair)
{
  inotify_rm_watch(fd, pair.first);
}


void interrupt(int param)
{
  std::cout<<std::endl;
  std::cout<< "MODIFIED FILES: " << std::endl;
  for_each(modified_file.begin(), modified_file.end(), print_file_path);
  for_each(wd_to_file_path.begin(), wd_to_file_path.end(), rm_watch);
  close(fd);
  exit(0);
}


void watch_addition(const fs::path file_path)
{
  if (!fs::exists(file_path))
  {
    std::cout<< "file or dir " << file_path.string() << " not found" << std::endl;
    return;
  }

  if (fs::is_directory(file_path))
  {
    fs::directory_iterator end_itr;
    for (fs::directory_iterator itr(file_path); itr != end_itr; ++itr)
      watch_addition(*itr);
    return;
  }

  int wd = inotify_add_watch(fd, file_path.c_str(), MASK); // watch descriptor
  wd_to_file_path.insert(std::make_pair(wd, file_path));
}


int main(int argc, const char* argv[])
{
  if (argc < 2)
  {
    std::cout<< "No arguments." << std::endl;
    std::cout<< "Valid arguments are file names and/or directory names." << std::endl;
    return 0;
  }

  fd = inotify_init(); // file descriptor

  if (fd < 0)
    perror("inotify init");
   
  for (int i = 1; i < argc; ++i)
    watch_addition(argv[i]);

  signal(SIGINT, interrupt); // Ctrl-C gives control to interrupt()

  const int MAX_EVENTS = 1024;
  const int LEN_NAME = 16;
  const int EVENT_SIZE = sizeof(inotify_event);
  const int BUF_SIZE = MAX_EVENTS * (EVENT_SIZE + LEN_NAME);
  char buffer[BUF_SIZE];

  while (true)
  {
    int length = read(fd, buffer, BUF_SIZE);
    if (length < 0)
      perror("read");

    int event_shift = 0;

    while (event_shift < length)
    {
      inotify_event* event = reinterpret_cast<inotify_event*>(& buffer[event_shift]);
    
      if (event->mask & MASK)
      {
        fs::path modified_file_path = wd_to_file_path[event->wd];
        std::cout<< "modified_file: " << modified_file_path << std::endl;
        modified_file.insert(modified_file_path);
      }
      event_shift += EVENT_SIZE + event->len;
    }  
  }

  return 0;
}
