package detalab.DTO;

import detalab.DTO.User;

public class LoggedUser extends User {

  //Instance
  private static LoggedUser instance;

  //Constructor
  private LoggedUser(String email, String username, String language) {
    super(email, username, language);
  }

  public static LoggedUser getInstance(User user) {
    if(instance == null)
      instance = new LoggedUser(user.getEmail(), user.getUsername(), user.getLanguage());
    return instance;
  }

  public static LoggedUser getInstance() {
    return instance;
  }

  public static void cleanUserSession() {
    instance = null;
  }

}