package main.java.detalab.DTO;

public class Response {
  private boolean result;
  private int status;
  private String message;
  
  public Response(boolean result, int status, String message) {
      this.result = result;
      this.status = status;
      this.message = message;
  }

  public boolean getResult() {
      return result;
  }

  public int getStatus() {
      return status;
  }

  public String getMessage() {
      return message;
  }
}
