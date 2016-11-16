#ifndef PROGRESS_STATE_H
#define PROGRESS_STATE_H

struct ProgressState
{
  enum {MAXCHAR = 16}           ;
  unsigned int current_test     ; //0-511 test number
  unsigned int total_test       ;//0-511 test number
  int seconds_remaining  ; //0-16383 seconds or about 4.5 hours

  char test_type[MAXCHAR];
  char subject_name[MAXCHAR];

  ProgressState() :
    current_test(0)
  , total_test(0)
  , seconds_remaining(0)
  {
    for (int i = 0; i < MAXCHAR; i++){
      {test_type[i] = 0;}
    }
    for (int j = 0; j < MAXCHAR; j++){
      {subject_name[j] = 0;} 
    }
  }
};

#endif // ndef PROGRESS_STATE_H
