  #ifndef ORV_PROGRESS_STATE_H
#define ORV_PROGRESS_STATE_H

struct ProgressState
{
  enum {MAXTEXT = 16}           ;
  unsigned short current_test     ; //0-511 test number
  unsigned short total_test       ;//0-511 test number
  unsigned short seconds_remaining  ; //0-16383 seconds or about 4.5 hours

  char subject_name[MAXTEXT];
  char test_type[MAXTEXT]; //max text length of 6 charaters

  ProgressState() :
    current_test(0)
  , total_test(0)
  , seconds_remaining(0)
  {
    for (int i = 0; i < MAXTEXT; i++){
      test_type[i] = 0;
    }
    for (int j = 0; j < MAXTEXT; j++){
      subject_name[j] = 0;
    }
  }
};



#endif // ndef ORV_PROGRESS_STATE_H
