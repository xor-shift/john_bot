create table if not exists clients_telegram (
  user_id bigint not null,
  token char(35) not null,

  primary key (user_id)
);

create table if not exists clients_irc (
  server varchar(255) not null,
  port int unsigned not null,

  username varchar(255) not null,
  realname varchar(255) not null
);

create table if not exists irc_nick_choices (
  irc_id int not null,
  choice_no int not null,

  nick varchar(255) not null,

  primary key (irc_id, choice_no),
  foreign key (irc_id) references clients_irc(id)
);

create table if not exists irc_channels (
  irc_id int not null,
  channel_no int not null,

  channel varchar(255) not null,

  primary key (irc_id, channel_no),
  foreign key (irc_id) references clients_irc(id)
);

create table if not exists relay_mappings (
  from_kv text not null,
  to_kv text not null
);

