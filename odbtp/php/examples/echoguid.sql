CREATE PROCEDURE EchoGUID
    @GUIDIn uniqueidentifier,
    @GUIDOut uniqueidentifier = NULL OUTPUT,
    @GUIDOutStr varchar(64) = NULL OUTPUT
AS
    SET NOCOUNT ON

    SET @GUIDOut = @GUIDIn
    SET @GUIDOutStr = CONVERT(varchar(64),@GUIDIn)
GO
