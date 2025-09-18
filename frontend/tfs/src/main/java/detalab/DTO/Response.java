package detalab.DTO;

public class Response {
  private boolean result;
  private int status;
  private String message;
  private String content;
  
  public Response(boolean result, int status, String message, String content) {
    this.result = result;
    this.status = status;
    this.message = message;
    this.content = content;
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

  public String getContent() {
    return content;
  }
}
