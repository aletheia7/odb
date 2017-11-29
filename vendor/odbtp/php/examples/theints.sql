CREATE TABLE dbo.TheInts (
	Id int IDENTITY (1, 1) NOT NULL ,
	TheTinyInt tinyint NOT NULL ,
	TheSmallInt smallint NOT NULL ,
	TheInt int NOT NULL ,
	TheBigInt bigint NOT NULL ,
	CONSTRAINT PKCL_TheInts_Id PRIMARY KEY  CLUSTERED (
		Id
	)
)
GO

CREATE PROCEDURE AddTheInts
    @TheTinyInt tinyint,
    @TheSmallInt smallint,
    @TheInt int,
    @TheBigInt bigint
AS
    SET NOCOUNT ON

    INSERT INTO TheInts( TheTinyInt, TheSmallInt, TheInt, TheBigInt )
                 VALUES( @TheTinyInt, @TheSmallInt, @TheInt, @TheBigInt )

    IF @@ERROR <> 0 RETURN 0
    RETURN @@IDENTITY
GO

CREATE PROCEDURE GetTheIntsString
    @Id int,
    @TheIntsString varchar(256) = NULL OUTPUT
AS
    SET NOCOUNT ON

    SET @TheIntsString =
     (SELECT 'Tiny Int = ' + CONVERT(varchar(32),TheTinyInt) + '  ' +
             'Small Int = ' + CONVERT(varchar(32),TheSmallInt) + '  ' +
             'Int = ' + CONVERT(varchar(32),TheInt) + '  ' +
             'Big Int = ' + CONVERT(varchar(32),TheBigInt)
      FROM TheInts WHERE Id = @Id)
GO

