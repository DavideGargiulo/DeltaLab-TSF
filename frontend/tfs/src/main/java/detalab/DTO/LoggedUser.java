package detalab.DTO;

public class LoggedUser extends User {

  // Instance
  private static LoggedUser instance;

  // Constructor
  private LoggedUser(int id, String email, String username, String language) {
    super(id, email, username, language);
  }

  public static LoggedUser getInstance(User user) {
    if (instance == null)
      instance = new LoggedUser(user.getId(), user.getEmail(), user.getUsername(), user.getLanguage());
    return instance;
  }

  public static LoggedUser getInstance() {
    return instance;
  }

  public static void cleanUserSession() {
    instance = null;
  }

}